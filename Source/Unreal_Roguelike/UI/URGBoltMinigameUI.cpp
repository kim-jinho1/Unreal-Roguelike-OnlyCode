#include "UI/URGBoltMinigameUI.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

void UURGBoltMinigameUI::Start()
{
	Super::Start();

	TightenedCount = 0;
	TurnedDegrees = 0.0f;
	bIsHolding = false;

	UpdateBoltVisual();
	UpdateProgressDots();

	if (TimeLimit > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(TimeLimitTimer, this, &UURGBoltMinigameUI::HandleTimeUp, TimeLimit, false);
	}
}

void UURGBoltMinigameUI::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimeLimitTimer);
	}

	Super::NativeDestruct();
}

FReply UURGBoltMinigameUI::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!IsRunning() || InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	const FVector2D MousePosition = InMouseEvent.GetScreenSpacePosition();

	bIsHolding = IsOverBolt(MousePosition);
	if (!bIsHolding)
	{
		return FReply::Handled();
	}

	LastAngle = GetAngleAround(MousePosition);
	MoveWrench(InGeometry, MousePosition, LastAngle);

	// 커서가 볼트를 벗어나도 Move/Up이 계속 오도록 캡처
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UURGBoltMinigameUI::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FVector2D MousePosition = InMouseEvent.GetScreenSpacePosition();

	if (!bIsHolding)
	{
		MoveWrench(InGeometry, MousePosition, 0.0f);
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	const float Angle = GetAngleAround(MousePosition);
	const float Delta = FMath::FindDeltaAngleDegrees(LastAngle, Angle);
	LastAngle = Angle;

	MoveWrench(InGeometry, MousePosition, Angle);

	// 시계 방향만 조여진다
	if (Delta <= 0.0f)
	{
		return FReply::Handled();
	}

	TurnedDegrees += Delta;

	if (TurnedDegrees < RequiredTurns * 360.0f)
	{
		UpdateBoltVisual();
		return FReply::Handled();
	}

	// 한 개 완료: 같은 볼트 이미지를 처음 상태로 돌려 다음 개로 쓴다. 손을 떼게 해서 다음 개를 새로 잡게 한다
	++TightenedCount;
	TurnedDegrees = 0.0f;
	bIsHolding = false;

	UpdateBoltVisual();
	UpdateProgressDots();

	if (TightenedCount >= BoltCount)
	{
		Finish(1.0f);
	}

	return FReply::Handled().ReleaseMouseCapture();
}

FReply UURGBoltMinigameUI::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}

	bIsHolding = false;
	MoveWrench(InGeometry, InMouseEvent.GetScreenSpacePosition(), 0.0f);

	return FReply::Handled().ReleaseMouseCapture();
}

FReply UURGBoltMinigameUI::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		Finish(GetProgress());
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

bool UURGBoltMinigameUI::IsOverBolt(const FVector2D& InScreenPosition) const
{
	if (BoltImage == nullptr)
	{
		return false;
	}

	// GetAbsoluteSize는 크기 벡터를 렌더 변환(회전 포함)으로 돌린 값이라 볼트가 돌수록 반경이 줄어든다.
	// 볼트 로컬 좌표에서 재면 회전·DPI 영향 없이 반경이 일정하다
	const FGeometry& Geometry = BoltImage->GetCachedGeometry();
	const FVector2D LocalSize = Geometry.GetLocalSize();
	const FVector2D LocalPosition = Geometry.AbsoluteToLocal(InScreenPosition);
	const float GrabRadius = FMath::Max(LocalSize.X, LocalSize.Y) * 0.5f * GrabRadiusScale;

	return FVector2D::Distance(LocalSize * 0.5f, LocalPosition) <= GrabRadius;
}

float UURGBoltMinigameUI::GetAngleAround(const FVector2D& InScreenPosition) const
{
	// GetAbsolutePosition은 좌상단 모서리를 렌더 변환한 점이라 볼트가 회전하면 중심에서 벗어난다.
	// 로컬 중심(0.5, 0.5)을 변환하면 중심 피벗 회전에 영향을 받지 않는다
	const FVector2D Center = BoltImage->GetCachedGeometry().GetAbsolutePositionAtCoordinates(FVector2D(0.5f, 0.5f));

	// 화면은 Y가 아래로 커지므로 Atan2 증가 방향이 곧 시계 방향
	const FVector2D Delta = InScreenPosition - Center;
	return FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
}

void UURGBoltMinigameUI::UpdateBoltVisual() const
{
	const float Progress = FMath::Clamp(TurnedDegrees / (RequiredTurns * 360.0f), 0.0f, 1.0f);

	if (BoltImage)
	{
		BoltImage->SetRenderTransformAngle(TurnedDegrees);
		BoltImage->SetColorAndOpacity(FMath::Lerp(LooseColor, TightColor, Progress));
	}

	if (BoltArc)
	{
		if (UMaterialInstanceDynamic* ArcMaterial = BoltArc->GetDynamicMaterial())
		{
			static const FName PercentParameterName(TEXT("Percent"));
			static const FName ColorParameterName(TEXT("Color"));

			ArcMaterial->SetScalarParameterValue(PercentParameterName, Progress);
			ArcMaterial->SetVectorParameterValue(ColorParameterName, ProgressColor);
		}
	}
}

void UURGBoltMinigameUI::UpdateProgressDots() const
{
	if (ProgressDots == nullptr)
	{
		return;
	}

	for (int32 Index = 0; Index < ProgressDots->GetChildrenCount(); ++Index)
	{
		UImage* Dot = Cast<UImage>(ProgressDots->GetChildAt(Index));
		if (Dot == nullptr)
		{
			continue;
		}

		if (Index >= BoltCount)
		{
			Dot->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		Dot->SetVisibility(ESlateVisibility::HitTestInvisible);
		Dot->SetColorAndOpacity(Index < TightenedCount ? ProgressColor : DotEmptyColor);
	}
}

void UURGBoltMinigameUI::MoveWrench(const FGeometry& InGeometry, const FVector2D& InScreenPosition, float InAngle) const
{
	if (WrenchImage == nullptr)
	{
		return;
	}

	if (UCanvasPanelSlot* WrenchSlot = Cast<UCanvasPanelSlot>(WrenchImage->Slot))
	{
		WrenchSlot->SetPosition(InGeometry.AbsoluteToLocal(InScreenPosition));
	}

	WrenchImage->SetRenderTransformAngle(InAngle);
}

float UURGBoltMinigameUI::GetProgress() const
{
	return BoltCount > 0 ? static_cast<float>(TightenedCount) / BoltCount : 0.0f;
}

void UURGBoltMinigameUI::HandleTimeUp()
{
	Finish(GetProgress());
}

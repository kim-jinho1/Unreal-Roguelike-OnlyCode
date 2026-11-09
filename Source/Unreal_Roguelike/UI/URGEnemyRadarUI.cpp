#include "UI/URGEnemyRadarUI.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UURGEnemyRadarUI::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 대상이 없으면 할 일이 없다 (레이더 부품이 없을 때 비용 0)
	if (Targets.IsEmpty())
	{
		return;
	}

	UpdateMarkers();
}

void UURGEnemyRadarUI::SetTargets(const TArray<FVector>& InTargets)
{
	Targets = InTargets;

	EnsureMarkers(Targets.Num());

	for (int32 Index = 0; Index < Markers.Num(); ++Index)
	{
		Markers[Index]->SetVisibility(Index < Targets.Num() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (!Targets.IsEmpty())
	{
		UpdateMarkers();
	}
}

void UURGEnemyRadarUI::EnsureMarkers(const int32 InCount)
{
	if (MarkerCanvas == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	while (Markers.Num() < InCount)
	{
		UImage* Marker = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		if (MarkerTexture)
		{
			Marker->SetBrushFromTexture(MarkerTexture);
		}
		Marker->SetColorAndOpacity(MarkerColor);

		// 캔버스 중앙 기준으로 놓고, 마커 자신의 중심을 기준점으로 회전시킨다
		if (UCanvasPanelSlot* MarkerSlot = MarkerCanvas->AddChildToCanvas(Marker))
		{
			MarkerSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			MarkerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			MarkerSlot->SetSize(MarkerSize);
		}

		Marker->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Markers.Add(Marker);
	}
}

void UURGEnemyRadarUI::UpdateMarkers()
{
	const APlayerController* PlayerController = GetOwningPlayer();
	const APawn* Pawn = GetOwningPlayerPawn();
	if (PlayerController == nullptr || Pawn == nullptr)
	{
		return;
	}

	// 3인칭 카메라는 컨트롤 회전을 따라가므로 그 yaw를 화면 위쪽으로 본다
	const float ViewYaw = PlayerController->GetControlRotation().Yaw;
	const FVector PawnLocation = Pawn->GetActorLocation();

	const int32 NumMarkers = FMath::Min(Targets.Num(), Markers.Num());
	for (int32 Index = 0; Index < NumMarkers; ++Index)
	{
		const FVector ToTarget = Targets[Index] - PawnLocation;
		const float RelativeYaw = FMath::FindDeltaAngleDegrees(ViewYaw, ToTarget.Rotation().Yaw);
		const float Radians = FMath::DegreesToRadians(RelativeYaw);

		// 위(-Y)가 정면, 오른쪽(+X)이 시계 방향 90°
		const FVector2D Offset(FMath::Sin(Radians) * MarkerRadius, -FMath::Cos(Radians) * MarkerRadius);

		if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(Markers[Index]->Slot))
		{
			MarkerSlot->SetPosition(Offset);
		}

		Markers[Index]->SetRenderTransformAngle(RelativeYaw);
	}
}


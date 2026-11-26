#include "UI/URGHitFeedbackUI.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
	/** 동시에 띄우는 데미지 숫자 상한 (미사일 다발이 한 번에 많이 맞아도 화면이 덮이지 않도록) */
	constexpr int32 MaxDamageNumbers = 24;

	/** 데미지 숫자가 이 비율만큼 지난 뒤부터 흐려진다 */
	constexpr float DamageNumberFadeStart = 0.6f;
}

void UURGHitFeedbackUI::AddHit(const FVector& InLocation, float InDamage, bool bInCrit, bool bInKilled)
{
	const float Now = GetFeedbackTime();

	LastHitTime = Now;
	if (bInKilled)
	{
		LastKillTime = Now;
	}

	// 다 떠오른 숫자는 여기서 정리한다 (적중이 없을 땐 페인트에서 건너뛰기만 한다)
	DamageNumbers.RemoveAll([this, Now](const FURGDamageNumber& Number)
	{
		return Now - Number.SpawnTime >= DamageNumberDuration;
	});

	if (DamageNumbers.Num() >= MaxDamageNumbers)
	{
		DamageNumbers.RemoveAt(0);
	}

	FURGDamageNumber& Number = DamageNumbers.AddDefaulted_GetRef();
	Number.WorldLocation = InLocation;
	Number.ScreenJitter = FVector2D(FMath::FRandRange(-16.0f, 16.0f), FMath::FRandRange(-8.0f, 8.0f));
	Number.Text = FText::AsNumber(FMath::Max(1, FMath::RoundToInt(InDamage)));
	Number.SpawnTime = Now;
	Number.bCrit = bInCrit;
}

int32 UURGHitFeedbackUI::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	const float Now = GetFeedbackTime();

	// 마커가 숫자 위에 오도록 한 층 위에 그린다
	const int32 NumberLayer = BaseLayer + 1;
	const int32 MarkerLayer = BaseLayer + 2;
	PaintDamageNumbers(AllottedGeometry, OutDrawElements, NumberLayer, Now);
	PaintHitMarker(AllottedGeometry, OutDrawElements, MarkerLayer, Now);

	return MarkerLayer;
}

void UURGHitFeedbackUI::PaintDamageNumbers(const FGeometry& InGeometry, FSlateWindowElementList& OutElements, int32 InLayerId, float InNow) const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController == nullptr || DamageNumbers.IsEmpty())
	{
		return;
	}

	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	for (const FURGDamageNumber& Number : DamageNumbers)
	{
		const float Progress = (InNow - Number.SpawnTime) / DamageNumberDuration;
		if (Progress < 0.0f || Progress >= 1.0f)
		{
			continue;
		}

		// 카메라 뒤에 있으면 투영이 실패한다
		FVector2D ScreenPosition;
		if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, Number.WorldLocation, ScreenPosition, true))
		{
			continue;
		}

		FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold",
			Number.bCrit ? FMath::RoundToInt(DamageNumberFontSize * CritFontScale) : DamageNumberFontSize);
		Font.OutlineSettings.OutlineSize = 2;

		const float Alpha = Progress < DamageNumberFadeStart ? 1.0f : 1.0f - (Progress - DamageNumberFadeStart) / (1.0f - DamageNumberFadeStart);
		Font.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.8f * Alpha);

		FLinearColor Color = Number.bCrit ? CritDamageNumberColor : DamageNumberColor;
		Color.A *= Alpha;

		// 처음에 빠르게 오르다 느려지도록 ease-out
		const float Rise = DamageNumberRise * (1.0f - FMath::Square(1.0f - Progress));
		const FVector2D TextSize = FontMeasure->Measure(Number.Text, Font);
		const FVector2D Position = ScreenPosition + Number.ScreenJitter - FVector2D(TextSize.X * 0.5f, TextSize.Y + Rise);

		FSlateDrawElement::MakeText(OutElements, InLayerId,
			InGeometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(Position)), Number.Text,
			Font, ESlateDrawEffect::None, Color);
	}
}

void UURGHitFeedbackUI::PaintHitMarker(const FGeometry& InGeometry, FSlateWindowElementList& OutElements, int32 InLayerId, float InNow) const
{
	// 처치 마커가 켜져 있으면 일반 적중이 이어져도 처치 마커를 유지한다
	const bool bKillMarker = InNow - LastKillTime < KillMarkerDuration;
	const float MarkerDuration = bKillMarker ? KillMarkerDuration : HitMarkerDuration;
	const float MarkerElapsed = InNow - (bKillMarker ? LastKillTime : LastHitTime);
	if (MarkerDuration <= 0.0f || MarkerElapsed >= MarkerDuration)
	{
		return;
	}

	const float Scale = bKillMarker ? KillMarkerScale : 1.0f;
	const float Inner = HitMarkerGap * Scale;
	const float Outer = Inner + HitMarkerLength * Scale;

	FLinearColor Color = bKillMarker ? KillMarkerColor : HitMarkerColor;
	Color.A *= 1.0f - MarkerElapsed / MarkerDuration;

	// 화면 중앙(크로스헤어)을 둘러싼 대각선 네 획
	const FVector2D Center = InGeometry.GetLocalSize() * 0.5f;
	const FVector2D Diagonals[] = { FVector2D(1.0f, 1.0f), FVector2D(1.0f, -1.0f), FVector2D(-1.0f, 1.0f), FVector2D(-1.0f, -1.0f) };
	for (const FVector2D& Diagonal : Diagonals)
	{
		const FVector2D Direction = Diagonal.GetSafeNormal();
		const TArray<FVector2D> Points = { Center + Direction * Inner, Center + Direction * Outer };
		FSlateDrawElement::MakeLines(OutElements, InLayerId, InGeometry.ToPaintGeometry(), Points,
			ESlateDrawEffect::None, Color, true, 2.5f * Scale);
	}
}

float UURGHitFeedbackUI::GetFeedbackTime() const
{
	const UWorld* World = GetWorld();
	return World != nullptr ? static_cast<float>(World->GetRealTimeSeconds()) : 0.0f;
}


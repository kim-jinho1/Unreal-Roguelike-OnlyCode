#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "URGHitFeedbackUI.generated.h"

/** 화면에 떠 있는 데미지 숫자 하나 */
struct FURGDamageNumber
{
	/** 맞은 월드 위치 (매 페인트마다 화면 좌표로 투영한다) */
	FVector WorldLocation = FVector::ZeroVector;

	/** 숫자가 겹치지 않도록 생성 시 정한 화면 오프셋 */
	FVector2D ScreenJitter = FVector2D::ZeroVector;

	FText Text;

	/** 생성 시각 (실제 초) */
	float SpawnTime = 0.0f;

	bool bCrit = false;
};

/**
 * 히트 마커(크로스헤어 둘레 X)와 데미지 숫자를 하위 위젯 없이 NativePaint에서 직접 그린다.
 * HUD 루트가 무기 적중 알림을 받아 AddHit으로 기록만 넘기고, 그리기는 이 위젯이 맡는다.
 * 화면 전체를 덮도록 배치해야 크로스헤어(화면 중앙)와 투영 위치가 맞는다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGHitFeedbackUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

public:
	/** 적중 기록: 히트 마커(처치면 처치 마커)를 켜고 데미지 숫자를 하나 추가한다 */
	void AddHit(const FVector& InLocation, float InDamage, bool bInCrit, bool bInKilled);

private:
	/** 맞은 자리에서 떠오르며 끝부분에 흐려지는 데미지 숫자를 그린다 */
	void PaintDamageNumbers(const FGeometry& InGeometry, FSlateWindowElementList& OutElements, int32 InLayerId, float InNow) const;

	/** 화면 중앙에 X자 네 획을 그린다. 표시 시간이 지났으면 그리지 않는다 */
	void PaintHitMarker(const FGeometry& InGeometry, FSlateWindowElementList& OutElements, int32 InLayerId, float InNow) const;

	/** 피드백 애니메이션 기준 시각. 시간 감속(전역 시간 배율)에 영향받지 않도록 실제 초 */
	float GetFeedbackTime() const;

protected:
	/** 히트 마커 표시 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "HitFeedbackUI|Marker", meta = (ClampMin = "0"))
	float HitMarkerDuration = 0.15f;

	/** 처치 마커 표시 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "HitFeedbackUI|Marker", meta = (ClampMin = "0"))
	float KillMarkerDuration = 0.3f;

	/** 화면 중앙에서 X 선이 시작되는 거리 (px) */
	UPROPERTY(EditDefaultsOnly, Category = "HitFeedbackUI|Marker", meta = (ClampMin = "0"))
	float HitMarkerGap = 8.0f;

	/** X 선 한 획의 길이 (px) */
	UPROPERTY(EditDefaultsOnly, Category = "HitFeedbackUI|Marker", meta = (ClampMin = "1"))
	float HitMarkerLength = 10.0f;

	/** 처치 마커 크기 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "HitFeedbackUI|Marker", meta = (ClampMin = "1"))
	float KillMarkerScale = 1.4f;

	UPROPERTY(EditDefaultsOnly, Category = "HitFeedbackUI|Marker")
	FLinearColor HitMarkerColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "HitFeedbackUI|Marker")
	FLinearColor KillMarkerColor = FLinearColor(1.0f, 0.15f, 0.1f, 1.0f);

	/** 데미지 숫자가 떠 있는 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "HitFeedbackUI|DamageNumber", meta = (ClampMin = "0.1"))
	float DamageNumberDuration = 0.7f;

	/** 데미지 숫자가 사라지기 전까지 떠오르는 높이 (px) */
	UPROPERTY(EditDefaultsOnly, Category = "HitFeedbackUI|DamageNumber", meta = (ClampMin = "0"))
	float DamageNumberRise = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "HitFeedbackUI|DamageNumber", meta = (ClampMin = "6"))
	int32 DamageNumberFontSize = 18;

	/** 치명타 숫자 크기 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "HitFeedbackUI|DamageNumber", meta = (ClampMin = "1"))
	float CritFontScale = 1.4f;

	UPROPERTY(EditDefaultsOnly, Category = "HitFeedbackUI|DamageNumber")
	FLinearColor DamageNumberColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "HitFeedbackUI|DamageNumber")
	FLinearColor CritDamageNumberColor = FLinearColor(1.0f, 0.85f, 0.1f, 1.0f);

private:
	/** 떠 있는 데미지 숫자 (적중 때 추가, 오래된 것은 다음 적중 때 정리) */
	TArray<FURGDamageNumber> DamageNumbers;

	/** 마지막 적중·처치 시각 (실제 초). 처음엔 아주 과거로 두어 마커가 꺼져 있게 한다 */
	float LastHitTime = -1000.0f;
	float LastKillTime = -1000.0f;

};


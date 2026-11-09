#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "URGHealthBarUI.generated.h"

/**
 * 플레이어 체력을 수치 + 바 또는 호(크로스헤어 둘레)로 표시하는 UI.
 * 호는 원형 진행 머티리얼(Percent·Color 파라미터)을 쓰며, 체력이 낮으면 경고 색으로 바뀐다
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGHealthBarUI : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 체력 바·호 비율 (0.0 ~ 1.0) */
	void SetRatio(float InRatio) const;

	/** 체력 수치 텍스트 ("현재 / 최대") */
	void SetText(float InCurrent, float InMax) const;

protected:
	/** 직선 체력 바. 호만 쓰는 WBP도 동작하도록 선택 바인딩 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UProgressBar> HealthBar;

	/** 원형 진행 머티리얼을 브러시로 쓰는 체력 호. 선택 바인딩 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UImage> HealthArc;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> HealthText;

private:
	/** 체력 텍스트 포맷 ({0} = 현재, {1} = 최대) */
	UPROPERTY(EditDefaultsOnly, Category = "HealthBarUI|Text")
	FText HealthTextFormat = NSLOCTEXT("URG.HealthBarUI", "HealthFormat", "{0} / {1}");

	/** 체력 호 기본 색 */
	UPROPERTY(EditDefaultsOnly, Category = "HealthBarUI|Arc")
	FLinearColor HealthColor = FLinearColor(0.3f, 0.82f, 0.48f, 1.0f);

	/** 체력이 낮을 때 체력 호 색 */
	UPROPERTY(EditDefaultsOnly, Category = "HealthBarUI|Arc")
	FLinearColor LowHealthColor = FLinearColor(1.0f, 0.15f, 0.1f, 1.0f);

	/** 이 비율 미만이면 체력 호를 경고 색으로 바꾼다 */
	UPROPERTY(EditDefaultsOnly, Category = "HealthBarUI|Arc", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowHealthRatio = 0.3f;

};


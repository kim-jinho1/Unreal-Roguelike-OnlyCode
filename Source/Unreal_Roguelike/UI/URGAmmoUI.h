#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "URGAmmoUI.generated.h"

/**
 * 주 무기 탄약을 "현재 / 탄창" 텍스트와 크로스헤어 둘레 호로 표시하는 UI.
 * 호는 원형 진행 머티리얼(Percent 파라미터)을 쓴다
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGAmmoUI : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 탄약 텍스트 갱신 (탄창 0이면 위젯 숨김) */
	void SetAmmo(int32 InCurrent, int32 InMagazineSize);

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> AmmoText;

	/** 원형 진행 머티리얼을 브러시로 쓰는 탄약 호. 선택 바인딩 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UImage> AmmoArc;

private:
	/** 탄약 텍스트 포맷 ({0} = 현재, {1} = 탄창) */
	UPROPERTY(EditDefaultsOnly, Category = "AmmoUI|Text")
	FText AmmoTextFormat = NSLOCTEXT("URG.AmmoUI", "AmmoFormat", "{0} / {1}");

};


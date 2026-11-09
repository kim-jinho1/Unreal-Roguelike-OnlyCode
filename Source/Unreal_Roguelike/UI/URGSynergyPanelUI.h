#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "URGSynergyPanelUI.generated.h"

struct FGameplayTag;

/**
 * 태그 시너지 패널 (부품 슬롯 위). 강화·열·전기 한 줄씩 고정 순서로 진행도와 효과를 보여 준다.
 * 하나도 모으지 않은 태그 줄은 숨고, 모든 줄이 숨으면 패널도 숨는다
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGSynergyPanelUI : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 태그 한 줄 반영. InText가 비어 있으면 그 줄을 숨긴다 */
	void SetSynergyRow(const FGameplayTag& InTag, const FText& InText, const FLinearColor& InColor);

private:
	/** 태그에 대응하는 줄 텍스트 (모르는 태그면 nullptr) */
	class UTextBlock* GetRowText(const FGameplayTag& InTag) const;

	/** 보이는 줄이 하나라도 있을 때만 패널을 보인다 */
	void RefreshPanelVisibility();

protected:
	/** 강화 시너지 줄 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> ReinforceRowText;

	/** 열 시너지 줄 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> ThermalRowText;

	/** 전기 시너지 줄 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> ElectricRowText;

};

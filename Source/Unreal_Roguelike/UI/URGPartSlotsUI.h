#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "URGDefine.h"
#include "URGPartSlotsUI.generated.h"

struct FURGPartSlotState;

/**
 * 부품 슬롯 4개(팔/다리/코어/머리)를 묶어 표시하는 UI
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGPartSlotsUI : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 해당 슬롯 위젯에 상태 반영 */
	void SetSlotState(EURGPartSlot InSlot, const FURGPartSlotState& InState) const;

	/** 해당 슬롯 위젯에 시너지 태그 표시 (InColor = 글자 색) */
	void SetSlotSynergyTags(EURGPartSlot InSlot, const FText& InTagText, const FLinearColor& InColor) const;

	/** 코어 슬롯 위젯에 코어 능력 지속·쿨다운 상태 반영 */
	void SetCoreAbilityState(bool bInActive, float InCooldownEndTime, float InCooldownDuration) const;

	/** 해당 슬롯 위젯에 지속 상태 없는 쿨다운 반영 (팔 공격·다리 대시) */
	void SetSlotCooldownState(EURGPartSlot InSlot, float InCooldownEndTime, float InCooldownDuration) const;

private:
	/** 슬롯에 대응하는 하위 위젯 */
	class UURGPartSlotUI* GetSlotUI(EURGPartSlot InSlot) const;

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UURGPartSlotUI> ArmSlotUI;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UURGPartSlotUI> LegSlotUI;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UURGPartSlotUI> CoreSlotUI;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UURGPartSlotUI> HeadSlotUI;

};


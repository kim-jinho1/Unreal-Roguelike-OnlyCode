#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "URGPartSlotUI.generated.h"

struct FURGPartSlotState;

/**
 * 부품 슬롯 1개를 표시하는 UI. 아이콘으로 장착 부품을, 둘레의 원형 호로 내구도를 나타낸다.
 * 호 색은 슬롯 색(픽업과 같은 색)이고, 호 길이가 남은 내구도 비율이다
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGPartSlotUI : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 슬롯 상태 반영. 장착 중이면 슬롯 색 호에 내구도 비율을 채우고, 빈 슬롯은 호를 빈 슬롯 색으로 비운다. 빈 팔·파손 팔은 근접 공격을 표시하고, 다른 빈 슬롯은 이름을 숨긴다. 파손 부품은 아이콘을 붉게 칠한다 */
	void SetSlotState(const FURGPartSlotState& InState, bool bInArmSlot = false);

	/** 시너지 태그 표시. 비어 있으면 숨긴다. 색은 HUD가 태그·발동 여부로 정한다 */
	void SetSynergyTags(const FText& InTagText, const FLinearColor& InColor) const;

	/** 슬롯 능력 상태 반영 (코어 능력·팔 공격·다리 대시). 지속 중이면 테두리를 켜고, 쿨다운 중이면 아이콘을 어둡게 한 뒤 남은 비율 호와 남은 초를 표시한다 */
	void SetAbilityState(bool bInActive, float InCooldownEndTime, float InCooldownDuration);

protected:
	virtual void NativeConstruct() override;

	/** 쿨다운 중에만 남은 시간 표시를 갱신한다. 끝나는 시각은 델리게이트로 한 번 받고, 화면 갱신만 여기서 한다 */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 남은 쿨다운(초)에 맞춰 호·숫자·아이콘 밝기·지속 테두리를 갱신한다. 0 이하면 쿨다운 표시를 끈다 */
	void UpdateAbilityVisuals(float InCooldownRemaining) const;

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UImage> PartIcon;

	/** 원형 진행 머티리얼(Percent·Color 파라미터)을 브러시로 쓰는 내구도 호. 선택 바인딩 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UImage> DurabilityArc;

	/** 부품 이름 (아이콘 아래). 없는 WBP도 동작하도록 선택 바인딩 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> PartNameText;

	/** 시너지 태그 이름 (부품 이름 아래). 선택 바인딩 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> SynergyTagText;

	/** 능력 쿨다운 호 (원형 진행 머티리얼, 남은 비율). 쿨다운이 없는 슬롯은 숨김. 선택 바인딩 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UImage> CooldownArc;

	/** 능력 남은 쿨다운 초. 선택 바인딩 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> CooldownText;

	/** 코어 능력 지속 중 테두리. 선택 바인딩 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UImage> ActiveGlow;

private:
	/** 장착 중일 때 내구도 호 색. 슬롯 위젯 인스턴스마다 지정한다 (팔 빨강 · 다리 초록 · 코어 파랑 · 머리 노랑) */
	UPROPERTY(EditAnywhere, Category = "PartSlotUI|Color")
	FLinearColor SlotColor = FLinearColor::White;

	/** 팔이 비었거나 파손되었을 때 사용하는 공격 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "PartSlotUI|Text")
	FText MeleeAttackText = NSLOCTEXT("URG.PartSlot", "MeleeAttack", "근접 공격");

	/** 파손 부품 이름을 유지하며 현재 공격을 안내한다 */
	UPROPERTY(EditDefaultsOnly, Category = "PartSlotUI|Text")
	FText BrokenArmNameFormat = NSLOCTEXT("URG.PartSlot", "BrokenArmName", "{0} · {1}");

	/** 빈 슬롯 호 색 */
	UPROPERTY(EditDefaultsOnly, Category = "PartSlotUI|Color")
	FLinearColor EmptyColor = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);

	/** 쿨다운 중 아이콘 색 (어둡게) */
	UPROPERTY(EditDefaultsOnly, Category = "PartSlotUI|Color")
	FLinearColor CooldownIconTint = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);

	/** 파손 부품 아이콘 색. 쿨다운보다 우선한다 (파손이면 능력도 못 쓴다) */
	UPROPERTY(EditDefaultsOnly, Category = "PartSlotUI|Color")
	FLinearColor BrokenIconTint = FLinearColor(1.0f, 0.1f, 0.05f, 1.0f);

	/** 쿨다운 호 색 */
	UPROPERTY(EditDefaultsOnly, Category = "PartSlotUI|Color")
	FLinearColor CooldownArcColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.8f);

	/** 능력 지속 중 여부 */
	bool bAbilityActive = false;

	/** 장착 부품이 파손 상태인지 (SetSlotState에서 갱신) */
	bool bBroken = false;

	/** 쿨다운이 끝나는 월드 초. 0이면 쿨다운 없음 */
	float CooldownEndTime = 0.0f;

	/** 이번 쿨다운의 전체 길이 (초) */
	float CooldownDuration = 0.0f;

};


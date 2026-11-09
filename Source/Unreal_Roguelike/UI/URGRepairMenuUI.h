#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "URGDefine.h"
#include "URGRepairMenuUI.generated.h"

/** 수리할 슬롯 선택 알림 (Slot) */
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateOnRepairSlotSelected, EURGPartSlot);

/** 메뉴 취소 알림 (Esc·우클릭) */
DECLARE_MULTICAST_DELEGATE(FDelegateOnRepairMenuCancelled);

/** 휠의 슬롯 한 칸 */
struct FURGRepairWheelEntry
{
	EURGPartSlot Slot = EURGPartSlot::Arm;
	TObjectPtr<class UURGPartSlotUI> SlotUI;
	bool bCanRepair = false;
	FText Label;
};

/**
 * 수리 장치의 슬롯 선택 휠. 위 머리 · 오른쪽 다리 · 아래 코어 · 왼쪽 팔로 부품 슬롯 4개를 배치하고,
 * 화면 중심에서 본 마우스 방향으로 슬롯을 고른다. 좌클릭은 확정, 우클릭·Esc는 취소.
 * 수리가 필요 없는 슬롯은 흐리게 보이고 고를 수 없다. 선택·취소는 델리게이트로 수리 장치에 알린다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGRepairMenuUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

public:
	/** 부품 컴포넌트의 슬롯 상태로 휠 4칸을 채운다 (빈 슬롯·멀쩡한 슬롯은 흐리게). 보유 자원이 비용보다 적으면 모든 칸을 막고 부족분을 안내한다 */
	void InitializeForPart(const class UURGPartComponent* InPartComponent, int32 InResource, int32 InRepairCost);

public:
	/** 슬롯을 골랐을 때 호출 */
	FDelegateOnRepairSlotSelected OnSlotSelected;

	/** 메뉴를 닫았을 때 호출 */
	FDelegateOnRepairMenuCancelled OnCancelled;

private:
	/** 위젯 중심 기준 마우스 방향의 휠 칸 인덱스. 중앙 데드존 안이면 INDEX_NONE */
	int32 FindEntryAt(const FGeometry& InGeometry, const FVector2D& InScreenPosition) const;

	/** 가리킨 칸 강조와 가운데 문구 갱신 */
	void SetHoveredEntry(int32 InIndex);

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UURGPartSlotUI> HeadSlot;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UURGPartSlotUI> LegSlot;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UURGPartSlotUI> CoreSlot;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UURGPartSlotUI> ArmSlot;

	/** 휠 가운데: 가리킨 슬롯의 이름·내구도, 없으면 안내 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> CenterText;

private:
	/** 이 반경(px, 위젯 로컬) 안은 아무 칸도 가리키지 않는다 */
	UPROPERTY(EditDefaultsOnly, Category = "RepairMenuUI|Select", meta = (ClampMin = "0"))
	float DeadZoneRadius = 60.0f;

	/** 가리킨 칸 확대 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "RepairMenuUI|Select", meta = (ClampMin = "1"))
	float HoverScale = 1.15f;

	/** 고를 수 없는 칸 불투명도 */
	UPROPERTY(EditDefaultsOnly, Category = "RepairMenuUI|Select", meta = (ClampMin = "0", ClampMax = "1"))
	float DisabledOpacity = 0.35f;

	/** 가운데 문구 포맷 ({0} = 부품 이름, {1} = 내구도 %) */
	UPROPERTY(EditDefaultsOnly, Category = "RepairMenuUI|Text")
	FText SlotTextFormat = NSLOCTEXT("URG.RepairMenu", "SlotFormat", "{0} {1}%");

	/** 빈 슬롯 문구 */
	UPROPERTY(EditDefaultsOnly, Category = "RepairMenuUI|Text")
	FText EmptySlotText = NSLOCTEXT("URG.RepairMenu", "EmptySlot", "빈 슬롯");

	/** 아무 칸도 가리키지 않을 때 가운데 안내 (무료 수리) */
	UPROPERTY(EditDefaultsOnly, Category = "RepairMenuUI|Text")
	FText SelectPromptText = NSLOCTEXT("URG.RepairMenu", "SelectPrompt", "수리할 부품");

	/** 아무 칸도 가리키지 않을 때 가운데 안내 ({0} = 비용, {1} = 보유 자원) */
	UPROPERTY(EditDefaultsOnly, Category = "RepairMenuUI|Text")
	FText CostPromptFormat = NSLOCTEXT("URG.RepairMenu", "CostPrompt", "수리할 부품\n자원 {0} 소모 (보유 {1})");

	/** 자원이 모자랄 때 가운데 안내 ({0} = 보유 자원, {1} = 비용) */
	UPROPERTY(EditDefaultsOnly, Category = "RepairMenuUI|Text")
	FText NotEnoughResourceFormat = NSLOCTEXT("URG.RepairMenu", "NotEnoughResource", "자원 부족\n{0} / {1}");

	/** 휠 칸. 위부터 시계 방향 (머리 · 다리 · 코어 · 팔) */
	TArray<FURGRepairWheelEntry> Entries;

	int32 HoveredIndex = INDEX_NONE;

	/** 아무 칸도 가리키지 않을 때 보여 줄 안내 (InitializeForPart에서 비용·보유 자원으로 정한다) */
	FText IdlePromptText;

};

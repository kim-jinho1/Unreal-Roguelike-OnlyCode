#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "URGDefine.h"
#include "URGPartCompareUI.generated.h"

struct FURGPartSlotState;
class UURGPartDataAsset;

/**
 * 부품 비교 UI (기획서: 부품에 접근하면 현재 부품과 새 부품을 함께 표시).
 * 크로스헤어 옆 작은 카드로 새 부품을 크게, 현재 부품은 이름·내구도 한 줄로 보여 준다.
 * 픽업을 바라보면 HUD가 ShowCompare로 채우고, 시선을 떼면 HideCompare로 숨긴다.
 * 부품 이름·설명·내구도·아이콘·슬롯 색은 이 위젯이 직접 만들고, 시너지 태그 이름과 시너지 변화 문구는 HUD가 만들어 넘긴다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGPartCompareUI : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 비교 내용 반영 후 표시.
	 * InNewTags는 새 부품의 태그 이름 목록 문구(없으면 빈 텍스트), InSynergyPreview는 장착 시 시너지 변화(없으면 빈 텍스트)
	 */
	void ShowCompare(const UURGPartDataAsset* InNewPart, const FURGPartSlotState& InCurrent, const FText& InNewTags, const FText& InSynergyPreview);

	/** 숨김 */
	void HideCompare();

private:
	/** 현재 부품 한 줄 (빈 슬롯이면 "빈 슬롯") */
	FText BuildCurrentText(const FURGPartSlotState& InCurrent) const;

	/** 슬롯 표시 이름 */
	const FText& GetSlotName(EURGPartSlot InSlot) const;

	/** 슬롯 색 (HUD 부품 슬롯과 같은 색) */
	const FLinearColor& GetSlotColor(EURGPartSlot InSlot) const;

	/** 아이콘 이미지 반영 (없으면 숨김) */
	void SetIcon(class UImage* InImage, class UTexture2D* InTexture) const;

protected:
	/** 슬롯 · 태그 (예: "다리 · 강화"), 슬롯 색 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> TitleText;

	/** 현재 부품 이름·내구도 한 줄 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> CurrentNameText;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UImage> NewIcon;

	/** 새 부품 아이콘 둘레 링 (슬롯 색) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UImage> NewRing;

	/** 카드 왼쪽 세로 줄 (슬롯 색) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UImage> AccentBar;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> NewNameText;

	/** 새 부품 설명 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> NewInfoText;

	/** 장착 시 시너지 변화. 변화가 없으면 숨긴다 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> SynergyPreviewText;

	/** 조작 안내 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> PromptText;

private:
	/** 제목 포맷 ({0} = 슬롯 이름, {1} = 태그 이름 목록). 태그가 없으면 슬롯 이름만 */
	UPROPERTY(EditDefaultsOnly, Category = "PartCompareUI|Text")
	FText TitleFormat = NSLOCTEXT("URG.PartCompare", "TitleFormat", "{0} · {1}");

	UPROPERTY(EditDefaultsOnly, Category = "PartCompareUI|Text")
	FText ArmSlotName = NSLOCTEXT("URG.PartCompare", "ArmSlot", "팔");

	UPROPERTY(EditDefaultsOnly, Category = "PartCompareUI|Text")
	FText LegSlotName = NSLOCTEXT("URG.PartCompare", "LegSlot", "다리");

	UPROPERTY(EditDefaultsOnly, Category = "PartCompareUI|Text")
	FText CoreSlotName = NSLOCTEXT("URG.PartCompare", "CoreSlot", "코어");

	UPROPERTY(EditDefaultsOnly, Category = "PartCompareUI|Text")
	FText HeadSlotName = NSLOCTEXT("URG.PartCompare", "HeadSlot", "머리");

	/** 현재 부품 줄 ({0} = 부품 이름, {1} = 내구도 %) */
	UPROPERTY(EditDefaultsOnly, Category = "PartCompareUI|Text")
	FText CurrentFormat = NSLOCTEXT("URG.PartCompare", "CurrentFormat", "현재 {0} {1}%");

	/** 빈 슬롯일 때 현재 부품 줄 */
	UPROPERTY(EditDefaultsOnly, Category = "PartCompareUI|Text")
	FText EmptySlotText = NSLOCTEXT("URG.PartCompare", "EmptySlot", "현재 빈 슬롯");

	/** 빈 슬롯에 장착할 때 안내 */
	UPROPERTY(EditDefaultsOnly, Category = "PartCompareUI|Text")
	FText EquipPrompt = NSLOCTEXT("URG.PartCompare", "EquipPrompt", "[E] 장착");

	/** 기존 부품과 교체할 때 안내 */
	UPROPERTY(EditDefaultsOnly, Category = "PartCompareUI|Text")
	FText SwapPrompt = NSLOCTEXT("URG.PartCompare", "SwapPrompt", "[E] 교체 — 기존 부품은 바닥에 떨어진다");

	UPROPERTY(EditDefaultsOnly, Category = "PartCompareUI|Color")
	FLinearColor ArmColor = FLinearColor(1.0f, 0.03f, 0.02f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "PartCompareUI|Color")
	FLinearColor LegColor = FLinearColor(0.02f, 0.7f, 0.06f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "PartCompareUI|Color")
	FLinearColor CoreColor = FLinearColor(0.02f, 0.2f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "PartCompareUI|Color")
	FLinearColor HeadColor = FLinearColor(1.0f, 0.55f, 0.0f, 1.0f);

};

#include "UI/URGPartCompareUI.h"
#include "Component/URGPartComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Equipment/URGPartDataAsset.h"
#include "Materials/MaterialInstanceDynamic.h"

void UURGPartCompareUI::ShowCompare(const UURGPartDataAsset* InNewPart, const FURGPartSlotState& InCurrent, const FText& InNewTags, const FText& InSynergyPreview)
{
	if (InNewPart == nullptr)
	{
		HideCompare();
		return;
	}

	const EURGPartSlot PartSlot = InNewPart->GetSlot();
	const FLinearColor& SlotColor = GetSlotColor(PartSlot);

	if (TitleText)
	{
		const FText& SlotName = GetSlotName(PartSlot);
		TitleText->SetText(InNewTags.IsEmpty() ? SlotName : FText::Format(TitleFormat, SlotName, InNewTags));
		TitleText->SetColorAndOpacity(FSlateColor(SlotColor));
	}

	if (AccentBar)
	{
		AccentBar->SetColorAndOpacity(SlotColor);
	}

	if (NewRing)
	{
		// 링은 호 머티리얼(한 바퀴)이라 색·채움을 파라미터로 준다. 새 부품은 항상 내구도 가득
		if (UMaterialInstanceDynamic* RingMaterial = NewRing->GetDynamicMaterial())
		{
			static const FName PercentParameterName(TEXT("Percent"));
			static const FName ColorParameterName(TEXT("Color"));

			RingMaterial->SetScalarParameterValue(PercentParameterName, 1.0f);
			RingMaterial->SetVectorParameterValue(ColorParameterName, SlotColor);
		}
	}

	SetIcon(NewIcon, InNewPart->GetIcon());

	if (NewNameText)
	{
		NewNameText->SetText(InNewPart->GetDisplayName());
	}

	if (NewInfoText)
	{
		NewInfoText->SetText(InNewPart->GetDescription());
	}

	if (CurrentNameText)
	{
		CurrentNameText->SetText(BuildCurrentText(InCurrent));
	}

	if (SynergyPreviewText)
	{
		SynergyPreviewText->SetText(InSynergyPreview);
		SynergyPreviewText->SetVisibility(InSynergyPreview.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (PromptText)
	{
		PromptText->SetText(InCurrent.IsEquipped() ? SwapPrompt : EquipPrompt);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UURGPartCompareUI::HideCompare()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

FText UURGPartCompareUI::BuildCurrentText(const FURGPartSlotState& InCurrent) const
{
	if (!InCurrent.IsEquipped())
	{
		return EmptySlotText;
	}

	// 손상된 부품을 버릴지가 판단 포인트이므로 이름 옆에 남은 내구도를 붙인다
	const float MaxDurability = InCurrent.Part->GetMaxDurability();
	const int32 DurabilityPercent = MaxDurability > 0.0f ? FMath::RoundToInt(InCurrent.Durability / MaxDurability * 100.0f) : 0;

	return FText::Format(CurrentFormat, InCurrent.GetDisplayName(), FText::AsNumber(DurabilityPercent));
}

const FText& UURGPartCompareUI::GetSlotName(const EURGPartSlot InSlot) const
{
	switch (InSlot)
	{
	case EURGPartSlot::Leg:
		return LegSlotName;
	case EURGPartSlot::Core:
		return CoreSlotName;
	case EURGPartSlot::Head:
		return HeadSlotName;
	default:
		return ArmSlotName;
	}
}

const FLinearColor& UURGPartCompareUI::GetSlotColor(const EURGPartSlot InSlot) const
{
	switch (InSlot)
	{
	case EURGPartSlot::Leg:
		return LegColor;
	case EURGPartSlot::Core:
		return CoreColor;
	case EURGPartSlot::Head:
		return HeadColor;
	default:
		return ArmColor;
	}
}

void UURGPartCompareUI::SetIcon(UImage* InImage, UTexture2D* InTexture) const
{
	if (InImage == nullptr)
	{
		return;
	}

	if (InTexture == nullptr)
	{
		InImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	InImage->SetBrushFromTexture(InTexture);
	InImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

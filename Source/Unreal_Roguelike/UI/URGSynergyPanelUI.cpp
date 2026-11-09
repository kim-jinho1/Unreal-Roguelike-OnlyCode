#include "UI/URGSynergyPanelUI.h"
#include "Components/TextBlock.h"
#include "GameplayTagContainer.h"
#include "URGGameplayTags.h"

void UURGSynergyPanelUI::SetSynergyRow(const FGameplayTag& InTag, const FText& InText, const FLinearColor& InColor)
{
	UTextBlock* RowText = GetRowText(InTag);
	if (RowText == nullptr)
	{
		return;
	}

	if (InText.IsEmpty())
	{
		RowText->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		RowText->SetText(InText);
		RowText->SetColorAndOpacity(FSlateColor(InColor));
		RowText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	RefreshPanelVisibility();
}

UTextBlock* UURGSynergyPanelUI::GetRowText(const FGameplayTag& InTag) const
{
	if (InTag.MatchesTagExact(URGGameplayTags::Part_Tag_Reinforce))
	{
		return ReinforceRowText;
	}

	if (InTag.MatchesTagExact(URGGameplayTags::Part_Tag_Thermal))
	{
		return ThermalRowText;
	}

	if (InTag.MatchesTagExact(URGGameplayTags::Part_Tag_Electric))
	{
		return ElectricRowText;
	}

	return nullptr;
}

void UURGSynergyPanelUI::RefreshPanelVisibility()
{
	const UTextBlock* Rows[] = { ReinforceRowText, ThermalRowText, ElectricRowText };

	bool bAnyVisible = false;
	for (const UTextBlock* Row : Rows)
	{
		if (Row && Row->GetVisibility() != ESlateVisibility::Collapsed)
		{
			bAnyVisible = true;
			break;
		}
	}

	SetVisibility(bAnyVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

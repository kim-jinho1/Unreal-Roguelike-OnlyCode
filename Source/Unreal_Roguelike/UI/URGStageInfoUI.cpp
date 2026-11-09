#include "UI/URGStageInfoUI.h"
#include "Components/TextBlock.h"

void UURGStageInfoUI::SetStageInfo(const int32 InStage, const int32 InRoom, const int32 InTotalRooms) const
{
	if (StageText)
	{
		StageText->SetText(FText::Format(
			StageTextFormat,
			FText::AsNumber(InStage),
			FText::AsNumber(InRoom),
			FText::AsNumber(InTotalRooms)));
	}
}


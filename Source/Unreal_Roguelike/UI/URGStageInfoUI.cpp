#include "UI/URGStageInfoUI.h"
#include "Components/TextBlock.h"

void UURGStageInfoUI::SetStageInfo(int32 InStage, int32 InRoom, int32 InTotalRooms) const
{
	if (StageText != nullptr)
	{
		StageText->SetText(FText::Format(
			StageTextFormat,
			FText::AsNumber(InStage),
			FText::AsNumber(InRoom),
			FText::AsNumber(InTotalRooms)));
	}
}


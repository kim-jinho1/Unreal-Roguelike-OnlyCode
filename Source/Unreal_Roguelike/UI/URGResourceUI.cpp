#include "UI/URGResourceUI.h"
#include "Components/TextBlock.h"

void UURGResourceUI::SetResource(int32 InResource) const
{
	if (ResourceText != nullptr)
	{
		ResourceText->SetText(FText::Format(ResourceTextFormat, FText::AsNumber(InResource)));
	}
}


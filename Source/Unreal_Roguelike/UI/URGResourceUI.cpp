#include "UI/URGResourceUI.h"
#include "Components/TextBlock.h"

void UURGResourceUI::SetResource(const int32 InResource) const
{
	if (ResourceText)
	{
		ResourceText->SetText(FText::Format(ResourceTextFormat, FText::AsNumber(InResource)));
	}
}


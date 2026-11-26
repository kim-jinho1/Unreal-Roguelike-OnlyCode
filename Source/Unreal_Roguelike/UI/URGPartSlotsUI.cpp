#include "UI/URGPartSlotsUI.h"
#include "UI/URGPartSlotUI.h"

void UURGPartSlotsUI::SetSlotState(EURGPartSlot InSlot, const FURGPartSlotState& InState) const
{
	if (UURGPartSlotUI* SlotUI = GetSlotUI(InSlot))
	{
		SlotUI->SetSlotState(InState, InSlot == EURGPartSlot::Arm);
	}
}

void UURGPartSlotsUI::SetSlotSynergyTags(EURGPartSlot InSlot, const FText& InTagText, const FLinearColor& InColor) const
{
	if (UURGPartSlotUI* SlotUI = GetSlotUI(InSlot))
	{
		SlotUI->SetSynergyTags(InTagText, InColor);
	}
}

void UURGPartSlotsUI::SetCoreAbilityState(bool bInActive, float InCooldownEndTime, float InCooldownDuration) const
{
	if (CoreSlotUI != nullptr)
	{
		CoreSlotUI->SetAbilityState(bInActive, InCooldownEndTime, InCooldownDuration);
	}
}

void UURGPartSlotsUI::SetSlotCooldownState(EURGPartSlot InSlot, float InCooldownEndTime, float InCooldownDuration) const
{
	if (UURGPartSlotUI* SlotUI = GetSlotUI(InSlot))
	{
		SlotUI->SetAbilityState(false, InCooldownEndTime, InCooldownDuration);
	}
}

UURGPartSlotUI* UURGPartSlotsUI::GetSlotUI(EURGPartSlot InSlot) const
{
	switch (InSlot)
	{
	case EURGPartSlot::Arm:
		return ArmSlotUI;

	case EURGPartSlot::Leg:
		return LegSlotUI;

	case EURGPartSlot::Core:
		return CoreSlotUI;

	case EURGPartSlot::Head:
		return HeadSlotUI;

	default:
		return nullptr;
	}
}


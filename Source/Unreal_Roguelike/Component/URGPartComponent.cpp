#include "Component/URGPartComponent.h"
#include "Equipment/URGPartDataAsset.h"
#include "GameplayTagContainer.h"

FText FURGPartSlotState::GetDisplayName() const
{
	return Part ? Part->GetDisplayName() : FText::GetEmpty();
}

UTexture2D* FURGPartSlotState::GetIcon() const
{
	return Part ? Part->GetIcon() : nullptr;
}

UURGPartComponent::UURGPartComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 슬롯 4개는 항상 존재한다 (빈 슬롯도 HUD에 표시)
	Slots.Add(EURGPartSlot::Arm, FURGPartSlotState());
	Slots.Add(EURGPartSlot::Leg, FURGPartSlotState());
	Slots.Add(EURGPartSlot::Core, FURGPartSlotState());
	Slots.Add(EURGPartSlot::Head, FURGPartSlotState());
}

void UURGPartComponent::BeginPlay()
{
	Super::BeginPlay();

	for (UURGPartDataAsset* Part : DefaultParts)
	{
		EquipPart(Part);
	}
}

const FURGPartSlotState& UURGPartComponent::GetSlotState(const EURGPartSlot InSlot) const
{
	static const FURGPartSlotState EmptyState;

	const FURGPartSlotState* State = Slots.Find(InSlot);
	return State ? *State : EmptyState;
}

bool UURGPartComponent::IsSlotUsable(const EURGPartSlot InSlot) const
{
	const FURGPartSlotState& State = GetSlotState(InSlot);
	return State.IsEquipped() && State.Condition != EURGPartCondition::Broken;
}

float UURGPartComponent::GetConditionScale(const EURGPartSlot InSlot) const
{
	const FURGPartSlotState& State = GetSlotState(InSlot);
	if (!State.IsEquipped())
	{
		return 0.0f;
	}

	switch (State.Condition)
	{
	case EURGPartCondition::Damaged:
		return DamagedScale;
	case EURGPartCondition::Broken:
		return 0.0f;
	default:
		return 1.0f;
	}
}

void UURGPartComponent::EquipPart(UURGPartDataAsset* InPart)
{
	if (InPart == nullptr)
	{
		return;
	}

	const EURGPartSlot Slot = InPart->GetSlot();

	FURGPartSlotState& State = Slots.FindOrAdd(Slot);
	State.Part = InPart;
	State.Condition = EURGPartCondition::Normal;
	State.Durability = InPart->GetMaxDurability();

	NotifySlotChanged(Slot);
}

void UURGPartComponent::RestoreSlots(const TMap<EURGPartSlot, FURGPartSlotState>& InSlots)
{
	// 빈 슬롯도 빠짐없이 알려야, BeginPlay에서 기본 부품으로 채워진 슬롯이 저장 당시의 빈 상태로 되돌아간다.
	const EURGPartSlot AllSlots[] = { EURGPartSlot::Arm, EURGPartSlot::Leg, EURGPartSlot::Core, EURGPartSlot::Head };
	for (const EURGPartSlot Slot : AllSlots)
	{
		const FURGPartSlotState* Saved = InSlots.Find(Slot);
		Slots.FindOrAdd(Slot) = Saved ? *Saved : FURGPartSlotState();

		NotifySlotChanged(Slot);
	}
}

void UURGPartComponent::UnequipPart(const EURGPartSlot InSlot)
{
	Slots.FindOrAdd(InSlot) = FURGPartSlotState();

	NotifySlotChanged(InSlot);
}

void UURGPartComponent::ApplyPartDamage(EURGPartSlot InSlot, float InDamage)
{
	FURGPartSlotState& State = Slots.FindOrAdd(InSlot);

	if (!State.IsEquipped() || State.Condition == EURGPartCondition::Broken || InDamage <= 0.0f)
	{
		return;
	}

	State.Durability = FMath::Max(0.0f, State.Durability - InDamage * PartDamageRatio * PartDamageScale);

	if (State.Durability <= 0.0f)
	{
		State.Condition = EURGPartCondition::Broken;
	}
	else if (State.Condition == EURGPartCondition::Normal && State.Durability <= State.Part->GetMaxDurability() * DamagedThreshold)
	{
		State.Condition = EURGPartCondition::Damaged;
	}

	NotifySlotChanged(InSlot);
}

void UURGPartComponent::RepairSlot(EURGPartSlot InSlot, float InAmount)
{
	FURGPartSlotState& State = Slots.FindOrAdd(InSlot);

	if (!State.IsEquipped() || InAmount <= 0.0f)
	{
		return;
	}

	const float MaxDurability = State.Part->GetMaxDurability();
	State.Durability = FMath::Min(MaxDurability, State.Durability + InAmount);

	// 올라가는 방향이라 임계치 통과 순간이 아니라 비율로 다시 정한다
	if (State.Durability > MaxDurability * DamagedThreshold)
	{
		State.Condition = EURGPartCondition::Normal;
	}
	else if (State.Durability > 0.0f)
	{
		State.Condition = EURGPartCondition::Damaged;
	}

	NotifySlotChanged(InSlot);
}

bool UURGPartComponent::NeedsRepair(EURGPartSlot InSlot) const
{
	const FURGPartSlotState& State = GetSlotState(InSlot);
	return State.IsEquipped() && State.Durability < State.Part->GetMaxDurability();
}

void UURGPartComponent::SetSlotCondition(const EURGPartSlot InSlot, const EURGPartCondition InCondition)
{
	FURGPartSlotState& State = Slots.FindOrAdd(InSlot);
	if (!State.IsEquipped() || State.Condition == InCondition)
	{
		return;
	}

	State.Condition = InCondition;
	NotifySlotChanged(InSlot);
}

int32 UURGPartComponent::GetSynergyTagCount(const FGameplayTag& InTag) const
{
	int32 Count = 0;

	for (const TPair<EURGPartSlot, FURGPartSlotState>& Pair : Slots)
	{
		// 파손 부품은 기능이 멈춘 것이므로 시너지에도 들어가지 않는다
		if (!IsSlotUsable(Pair.Key))
		{
			continue;
		}

		// HasTag는 부모 태그도 일치로 보므로 정확히 그 태그만 센다
		if (Pair.Value.Part->GetSynergyTags().HasTagExact(InTag))
		{
			++Count;
		}
	}

	return Count;
}

bool UURGPartComponent::IsSynergyActive(const FGameplayTag& InTag) const
{
	return GetSynergyTagCount(InTag) >= SynergyThreshold;
}

bool UURGPartComponent::IsSynergyActiveWithPart(const FGameplayTag& InTag, const UURGPartDataAsset* InPart) const
{
	if (InPart == nullptr)
	{
		return IsSynergyActive(InTag);
	}

	// 새 부품이 들어갈 슬롯의 기존 부품은 빼고, 새 부품은 정상 상태로 들어오므로 그대로 센다
	const EURGPartSlot TargetSlot = InPart->GetSlot();
	int32 Count = InPart->GetSynergyTags().HasTagExact(InTag) ? 1 : 0;

	for (const TPair<EURGPartSlot, FURGPartSlotState>& Pair : Slots)
	{
		if (Pair.Key == TargetSlot || !IsSlotUsable(Pair.Key))
		{
			continue;
		}

		if (Pair.Value.Part->GetSynergyTags().HasTagExact(InTag))
		{
			++Count;
		}
	}

	return Count >= SynergyThreshold;
}

void UURGPartComponent::NotifySlotChanged(const EURGPartSlot InSlot) const
{
	OnSlotChanged.Broadcast(InSlot, GetSlotState(InSlot));
}


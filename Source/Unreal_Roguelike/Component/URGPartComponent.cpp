#include "Component/URGPartComponent.h"
#include "Equipment/URGPartDataAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/HitResult.h"

FText FURGPartSlotState::GetDisplayName() const
{
	return Part != nullptr ? Part->GetDisplayName() : FText::GetEmpty();
}

UTexture2D* FURGPartSlotState::GetIcon() const
{
	return Part != nullptr ? Part->GetIcon() : nullptr;
}

UURGPartComponent::UURGPartComponent()
{
	// 장착·피격·수리·복원 때만 상태가 바뀌고 델리게이트로 알리므로 Tick은 필요하지 않다.
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

const FURGPartSlotState& UURGPartComponent::GetSlotState(EURGPartSlot InSlot) const
{
	static const FURGPartSlotState EmptyState;

	const FURGPartSlotState* State = Slots.Find(InSlot);
	return State != nullptr ? *State : EmptyState;
}

bool UURGPartComponent::IsSlotUsable(EURGPartSlot InSlot) const
{
	const FURGPartSlotState& State = GetSlotState(InSlot);
	return State.IsEquipped() && State.Condition != EURGPartCondition::Broken;
}

float UURGPartComponent::GetConditionScale(EURGPartSlot InSlot) const
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
		Slots.FindOrAdd(Slot) = Saved != nullptr ? *Saved : FURGPartSlotState();

		NotifySlotChanged(Slot);
	}
}

void UURGPartComponent::UnequipPart(EURGPartSlot InSlot)
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

void UURGPartComponent::SetSlotCondition(EURGPartSlot InSlot, EURGPartCondition InCondition)
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
	return CountSynergyTag(InTag, nullptr);
}

bool UURGPartComponent::IsSynergyActiveWithPart(const FGameplayTag& InTag, const UURGPartDataAsset* InPart) const
{
	return CountSynergyTag(InTag, InPart) >= SynergyThreshold;
}

int32 UURGPartComponent::CountSynergyTag(const FGameplayTag& InTag, const UURGPartDataAsset* InReplacePart) const
{
	// 새 부품이 들어갈 슬롯의 기존 부품은 빼고, 새 부품은 정상 상태로 들어오므로 그대로 센다
	int32 Count = 0;
	if (InReplacePart != nullptr && InReplacePart->GetSynergyTags().HasTagExact(InTag))
	{
		++Count;
	}

	for (const TPair<EURGPartSlot, FURGPartSlotState>& Pair : Slots)
	{
		if (InReplacePart != nullptr && Pair.Key == InReplacePart->GetSlot())
		{
			continue;
		}

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

bool UURGPartComponent::ResolveSlotFromHit(const FHitResult& InHit, const USkeletalMeshComponent* InMesh, EURGPartSlot& OutSlot) const
{
	if (InMesh == nullptr)
	{
		return false;
	}

	FName Bone = InHit.BoneName;

	// 캡슐에 맞으면 뼈 이름이 비어 오므로 충돌점에서 가장 가까운 바디 뼈로 대신한다.
	if (Bone.IsNone())
	{
		Bone = InMesh->FindClosestBone(InHit.ImpactPoint, nullptr, 0.0f, /*bRequirePhysicalAsset*/ true);
	}

	while (!Bone.IsNone())
	{
		if (const EURGPartSlot* Found = HitBoneSlotMap.Find(Bone))
		{
			OutSlot = *Found;
			return true;
		}

		Bone = InMesh->GetParentBone(Bone);
	}

	return false;
}

void UURGPartComponent::NotifySlotChanged(EURGPartSlot InSlot)
{
	// 시너지 후보: 지금 쓸 수 있는 부품의 태그 + 이미 켜져 있던 태그 (꺼지는 것도 잡아야 한다)
	FGameplayTagContainer Candidates = ActiveSynergies;
	for (const TPair<EURGPartSlot, FURGPartSlotState>& Pair : Slots)
	{
		if (IsSlotUsable(Pair.Key))
		{
			Candidates.AppendTags(Pair.Value.Part->GetSynergyTags());
		}
	}

	// 기록부터 고친다. 슬롯 핸들러(HUD 태그 색 등)가 이 기록을 읽기 때문
	TArray<TPair<FGameplayTag, bool>> ChangedSynergies;
	for (const FGameplayTag& Tag : Candidates)
	{
		const bool bActive = CountSynergyTag(Tag, nullptr) >= SynergyThreshold;
		if (ActiveSynergies.HasTagExact(Tag) == bActive)
		{
			continue;
		}

		if (bActive)
		{
			ActiveSynergies.AddTag(Tag);
		}
		else
		{
			ActiveSynergies.RemoveTag(Tag);
		}

		ChangedSynergies.Emplace(Tag, bActive);
	}

	OnSlotChanged.Broadcast(InSlot, GetSlotState(InSlot));

	// 켜지고 꺼진 순간만 알린다 (HUD 문구가 반복되지 않고, 받는 쪽이 더하고 빼기를 짝 맞춰 할 수 있도록)
	for (const TPair<FGameplayTag, bool>& Changed : ChangedSynergies)
	{
		OnSynergyChanged.Broadcast(Changed.Key, Changed.Value);
	}
}


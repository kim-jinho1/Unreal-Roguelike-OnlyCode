#include "UI/URGRepairMenuUI.h"
#include "Component/URGPartComponent.h"
#include "Components/TextBlock.h"
#include "Equipment/URGPartDataAsset.h"
#include "UI/URGPartSlotUI.h"

void UURGRepairMenuUI::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 기본 가시성(SelfHitTestInvisible)으로는 위젯 자체가 마우스 이벤트를 받지 못한다
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	// 방향 계산이 위에서부터 시계 방향 구역이므로 같은 순서로 담는다
	const TPair<EURGPartSlot, UURGPartSlotUI*> Layout[] = {
		{ EURGPartSlot::Head, HeadSlot },
		{ EURGPartSlot::Leg, LegSlot },
		{ EURGPartSlot::Core, CoreSlot },
		{ EURGPartSlot::Arm, ArmSlot },
	};

	for (const TPair<EURGPartSlot, UURGPartSlotUI*>& Item : Layout)
	{
		FURGRepairWheelEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.Slot = Item.Key;
		Entry.SlotUI = Item.Value;
	}
}

FReply UURGRepairMenuUI::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		OnCancelled.Broadcast();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UURGRepairMenuUI::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	SetHoveredEntry(FindEntryAt(InGeometry, InMouseEvent.GetScreenSpacePosition()));
	return FReply::Handled();
}

FReply UURGRepairMenuUI::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnCancelled.Broadcast();
		return FReply::Handled();
	}

	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	// 마우스를 움직이지 않고 바로 클릭할 수도 있으므로 클릭 위치로 다시 계산한다
	const int32 Index = FindEntryAt(InGeometry, InMouseEvent.GetScreenSpacePosition());
	SetHoveredEntry(Index);

	if (Entries.IsValidIndex(Index) && Entries[Index].bCanRepair)
	{
		OnSlotSelected.Broadcast(Entries[Index].Slot);
	}

	return FReply::Handled();
}

void UURGRepairMenuUI::InitializeForPart(const UURGPartComponent* InPartComponent, const int32 InResource, const int32 InRepairCost)
{
	if (InPartComponent == nullptr)
	{
		return;
	}

	const bool bCanAfford = InResource >= InRepairCost;
	if (InRepairCost <= 0)
	{
		IdlePromptText = SelectPromptText;
	}
	else if (bCanAfford)
	{
		IdlePromptText = FText::Format(CostPromptFormat, FText::AsNumber(InRepairCost), FText::AsNumber(InResource));
	}
	else
	{
		IdlePromptText = FText::Format(NotEnoughResourceFormat, FText::AsNumber(InResource), FText::AsNumber(InRepairCost));
	}

	for (FURGRepairWheelEntry& Entry : Entries)
	{
		const FURGPartSlotState& State = InPartComponent->GetSlotState(Entry.Slot);
		// 자원이 모자라면 고를 수 없게 막는다 (수리 장치가 슬롯 선택 시 차감한다)
		Entry.bCanRepair = bCanAfford && InPartComponent->NeedsRepair(Entry.Slot);

		if (State.IsEquipped())
		{
			const float MaxDurability = State.Part->GetMaxDurability();
			const int32 Percent = MaxDurability > 0.0f ? FMath::RoundToInt(State.Durability / MaxDurability * 100.0f) : 0;
			Entry.Label = FText::Format(SlotTextFormat, State.GetDisplayName(), FText::AsNumber(Percent));
		}
		else
		{
			Entry.Label = EmptySlotText;
		}

		if (Entry.SlotUI)
		{
			Entry.SlotUI->SetSlotState(State);
			Entry.SlotUI->SetRenderOpacity(Entry.bCanRepair ? 1.0f : DisabledOpacity);
			Entry.SlotUI->SetRenderScale(FVector2D::UnitVector);
		}
	}

	HoveredIndex = INDEX_NONE;

	if (CenterText)
	{
		CenterText->SetText(IdlePromptText);
	}
}

int32 UURGRepairMenuUI::FindEntryAt(const FGeometry& InGeometry, const FVector2D& InScreenPosition) const
{
	if (Entries.Num() == 0)
	{
		return INDEX_NONE;
	}

	const FVector2D Offset = InGeometry.AbsoluteToLocal(InScreenPosition) - InGeometry.GetLocalSize() * 0.5f;
	if (Offset.SizeSquared() < FMath::Square(DeadZoneRadius))
	{
		return INDEX_NONE;
	}

	// 위를 0도로 시계 방향 증가 (화면 Y는 아래로 커지므로 -Y가 위)
	const float Degrees = FMath::Fmod(FMath::RadiansToDegrees(FMath::Atan2(Offset.X, -Offset.Y)) + 360.0f, 360.0f);
	const float SectorSize = 360.0f / Entries.Num();

	// 각 칸이 자기 방향을 가운데로 둔 구역을 차지하도록 반 구역 밀어서 나눈다
	return FMath::FloorToInt((Degrees + SectorSize * 0.5f) / SectorSize) % Entries.Num();
}

void UURGRepairMenuUI::SetHoveredEntry(int32 InIndex)
{
	if (InIndex == HoveredIndex)
	{
		return;
	}

	if (Entries.IsValidIndex(HoveredIndex) && Entries[HoveredIndex].SlotUI)
	{
		Entries[HoveredIndex].SlotUI->SetRenderScale(FVector2D::UnitVector);
	}

	HoveredIndex = InIndex;

	if (!Entries.IsValidIndex(HoveredIndex))
	{
		if (CenterText)
		{
			CenterText->SetText(IdlePromptText);
		}
		return;
	}

	const FURGRepairWheelEntry& Entry = Entries[HoveredIndex];

	// 고를 수 없는 칸도 상태는 보여 주되, 확대는 고를 수 있는 칸만 해서 클릭이 먹힐지 구분되게 한다
	if (Entry.SlotUI && Entry.bCanRepair)
	{
		Entry.SlotUI->SetRenderScale(FVector2D(HoverScale));
	}

	if (CenterText)
	{
		CenterText->SetText(Entry.Label);
	}
}

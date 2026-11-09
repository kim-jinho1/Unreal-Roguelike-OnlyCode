#include "UI/URGGameOverUI.h"
#include "Component/URGPartComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Equipment/URGPartDataAsset.h"
#include "UI/URGPartSlotUI.h"

void UURGGameOverUI::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 마우스 없이도 입력 포커스를 받을 수 있게 한다
	SetIsFocusable(true);

	if (RetryButton)
	{
		RetryButton->OnClicked.AddDynamic(this, &UURGGameOverUI::HandleRetryClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UURGGameOverUI::HandleQuitClicked);
	}

	if (TitleButton)
	{
		TitleButton->OnClicked.AddDynamic(this, &UURGGameOverUI::HandleTitleClicked);
	}
}

void UURGGameOverUI::SetProgress(const int32 InChapter, const int32 InRoom, const int32 InTotalRooms) const
{
	if (ProgressText)
	{
		ProgressText->SetText(FText::Format(
			ProgressTextFormat,
			FText::AsNumber(InChapter),
			FText::AsNumber(InRoom),
			FText::AsNumber(InTotalRooms)));
	}
}

void UURGGameOverUI::SetPartSlots(const UURGPartComponent* InPartComponent) const
{
	if (InPartComponent == nullptr)
	{
		return;
	}

	SetupSlot(ArmSlot, ArmText, InPartComponent, EURGPartSlot::Arm);
	SetupSlot(LegSlot, LegText, InPartComponent, EURGPartSlot::Leg);
	SetupSlot(CoreSlot, CoreText, InPartComponent, EURGPartSlot::Core);
	SetupSlot(HeadSlot, HeadText, InPartComponent, EURGPartSlot::Head);
}

void UURGGameOverUI::HandleRetryClicked()
{
	// 맵 이동 전에 한 번 더 눌려 이동 요청이 겹치지 않게 막는다
	SetIsEnabled(false);
	OnRetryRequested.Broadcast();
}

void UURGGameOverUI::HandleQuitClicked()
{
	SetIsEnabled(false);
	OnQuitRequested.Broadcast();
}

void UURGGameOverUI::HandleTitleClicked()
{
	SetIsEnabled(false);
	OnTitleRequested.Broadcast();
}

void UURGGameOverUI::SetupSlot(UURGPartSlotUI* InSlotUI, UTextBlock* InText, const UURGPartComponent* InPartComponent, const EURGPartSlot InSlot) const
{
	const FURGPartSlotState& State = InPartComponent->GetSlotState(InSlot);

	if (InSlotUI)
	{
		InSlotUI->SetSlotState(State, InSlot == EURGPartSlot::Arm);
	}

	if (InText == nullptr)
	{
		return;
	}

	if (!State.IsEquipped())
	{
		InText->SetText(EmptySlotText);
		InText->SetColorAndOpacity(FSlateColor(NormalTextColor));
		return;
	}

	// 사망 원인을 짚어 주는 게 이 화면의 목적이라 파손은 퍼센트 대신 글자와 색으로 강조한다
	if (State.Condition == EURGPartCondition::Broken)
	{
		InText->SetText(FText::Format(BrokenTextFormat, State.GetDisplayName()));
		InText->SetColorAndOpacity(FSlateColor(BrokenTextColor));
		return;
	}

	const float MaxDurability = State.Part->GetMaxDurability();
	const int32 Percent = MaxDurability > 0.0f ? FMath::RoundToInt(State.Durability / MaxDurability * 100.0f) : 0;

	InText->SetText(FText::Format(SlotTextFormat, State.GetDisplayName(), FText::AsNumber(Percent)));
	InText->SetColorAndOpacity(FSlateColor(NormalTextColor));
}

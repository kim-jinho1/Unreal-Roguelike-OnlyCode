#include "UI/URGCommLogUI.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UURGCommLogUI::NativeConstruct()
{
	Super::NativeConstruct();

	// 상황 발생 전에는 보이지 않는다
	SetVisibility(ESlateVisibility::Collapsed);
}

void UURGCommLogUI::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimer);
	}

	Super::NativeDestruct();
}

void UURGCommLogUI::ShowMessage(const FText& InMessage, const float InDuration)
{
	if (MessageText)
	{
		MessageText->SetText(InMessage);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// 새 메시지가 오면 이전 타이머는 무효 (마지막 메시지 기준으로 숨긴다)
	World->GetTimerManager().ClearTimer(HideTimer);
	if (InDuration > 0.0f)
	{
		World->GetTimerManager().SetTimer(HideTimer, this, &UURGCommLogUI::HideMessage, InDuration, false);
	}
}

void UURGCommLogUI::HideMessage()
{
	SetVisibility(ESlateVisibility::Collapsed);
}


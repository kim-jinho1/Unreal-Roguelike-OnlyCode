#include "UI/URGMinigameUI.h"

void UURGMinigameUI::Start()
{
	bIsRunning = true;

	// 기본 가시성(SelfHitTestInvisible)으로는 위젯 자체가 마우스 이벤트를 받지 못한다
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);
	AddToViewport();
}

void UURGMinigameUI::Finish(float InScore)
{
	if (!bIsRunning)
	{
		return;
	}

	bIsRunning = false;
	RemoveFromParent();

	OnFinished.Broadcast(FMath::Clamp(InScore, 0.0f, 1.0f));
}

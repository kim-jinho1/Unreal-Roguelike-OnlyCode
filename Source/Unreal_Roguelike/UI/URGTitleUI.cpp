#include "UI/URGTitleUI.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UURGTitleUI::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 마우스 없이도 입력 포커스를 받을 수 있게 한다
	SetIsFocusable(true);

	if (TitleText)
	{
		TitleText->SetText(TitleLabel);
	}

	if (StartButton)
	{
		StartButton->OnClicked.AddDynamic(this, &UURGTitleUI::HandleStartClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UURGTitleUI::HandleQuitClicked);
	}
}

void UURGTitleUI::HandleStartClicked()
{
	// 맵 이동 전에 한 번 더 눌려 이동 요청이 겹치지 않게 막는다
	SetIsEnabled(false);
	OnStartRequested.Broadcast();
}

void UURGTitleUI::HandleQuitClicked()
{
	SetIsEnabled(false);
	OnQuitRequested.Broadcast();
}


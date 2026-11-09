#include "Core/URGTitlePlayerController.h"
#include "Core/URGGameInstance.h"
#include "UI/URGTitleUI.h"
#include "Kismet/KismetSystemLibrary.h"

void AURGTitlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController() || TitleWidgetClass == nullptr)
	{
		return;
	}

	TitleWidget = CreateWidget<UURGTitleUI>(this, TitleWidgetClass);
	if (TitleWidget == nullptr)
	{
		return;
	}

	TitleWidget->OnStartRequested.AddUObject(this, &AURGTitlePlayerController::HandleStartRequested);
	TitleWidget->OnQuitRequested.AddUObject(this, &AURGTitlePlayerController::HandleQuitRequested);
	TitleWidget->AddToViewport();

	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(TitleWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AURGTitlePlayerController::HandleStartRequested()
{
	UURGGameInstance* GameInstance = GetGameInstance<UURGGameInstance>();
	if (GameInstance == nullptr)
	{
		return;
	}

	// 입력 모드는 뷰포트에 남아 맵을 옮겨도 유지되므로, 이동 전에 게임 입력으로 되돌린다
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());

	// 새 런 시작: 챕터·진행도·저장된 기체 상태를 초기화하고 스테이지 1 풀에서 맵을 고른다
	if (GameInstance->RequestRestartRun())
	{
		return;
	}

	// 이동에 실패하면(스테이지 풀이 비어 있는 등) 화면에 갇히지 않게 타이틀 조작을 되살린다
	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());
	if (TitleWidget)
	{
		TitleWidget->SetIsEnabled(true);
	}
}

void AURGTitlePlayerController::HandleQuitRequested()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}


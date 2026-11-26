#include "Core/URGPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Component/URGAttributeComponent.h"
#include "Component/URGPartComponent.h"
#include "Component/URGStageTransitionComponent.h"
#include "Core/URGGameInstance.h"
#include "UI/URGGameOverUI.h"
#include "UI/URGPlayerHUD.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

AURGPlayerController::AURGPlayerController()
{
	StageTransitionComponent = CreateDefaultSubobject<UURGStageTransitionComponent>(TEXT("StageTransitionComponent"));
}

void AURGPlayerController::BeginPlay()
{
	// 도착 화면 연출과 출구 구독은 StageTransitionComponent가 자기 BeginPlay에서 한다
	Super::BeginPlay();

	if (IsLocalController())
	{
		if (UURGGameInstance* GameInstance = GetGameInstance<UURGGameInstance>())
		{
			GameInstance->OnRunCompleted.AddUObject(this, &AURGPlayerController::HandleRunCompleted);
		}
	}

	// 실제 셋업은 OnPossess에서 일괄 처리한다.
	// BeginPlay 시점에 이미 폰을 소유 중인데 OnPossess를 놓친 경우만 보강한다.
	if (GetPawn() != nullptr && !bHUDInitialized)
	{
		SetupHUD();
		BindToPawn(GetPawn());
	}
}

void AURGPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	SetupHUD();
	BindToPawn(InPawn);
}

void AURGPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(GameOverTimer);

	// 게임 인스턴스는 맵이 바뀌어도 살아남으므로, 사라지는 컨트롤러를 계속 부르지 않게 해제한다
	if (UURGGameInstance* GameInstance = GetGameInstance<UURGGameInstance>())
	{
		GameInstance->OnRunCompleted.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AURGPlayerController::RequestPause()
{
	OnPauseRequested.Broadcast();
}

void AURGPlayerController::OpenModalWidget(UUserWidget* InWidget)
{
	if (InWidget == nullptr)
	{
		return;
	}

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(InWidget->TakeWidget());
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void AURGPlayerController::CloseModalWidget()
{
	// 모달을 연 쪽도 OnDeath를 받아 세션을 닫으므로 사망 처리와 순서 없이 올 수 있다. 어느 순서든 게임오버 입력이 남게 한다
	if (bIsPlayerDead)
	{
		return;
	}

	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void AURGPlayerController::SetupHUD()
{
	if (!IsLocalController() || HUDWidget != nullptr || HUDWidgetClass == nullptr)
	{
		return;
	}

	HUDWidget = CreateWidget<UURGPlayerHUD>(this, HUDWidgetClass);
	if (HUDWidget == nullptr)
	{
		return;
	}

	HUDWidget->AddToViewport();
}

void AURGPlayerController::BindToPawn(APawn* InPawn)
{
	if (InPawn == nullptr)
	{
		return;
	}

	// HUD 폰 바인딩은 한 번만 (OnPossess/BeginPlay 중복 호출 대비)
	if (HUDWidget != nullptr && !bHUDInitialized)
	{
		HUDWidget->InitializeForPawn(InPawn);

		// 스테이지 진행도는 폰이 아니라 게임 인스턴스가 들고 있다 (맵이 바뀌어도 살아남는 쪽).
		if (const UURGGameInstance* GameInstance = GetGameInstance<UURGGameInstance>())
		{
			int32 Chapter = 0;
			int32 Room = 0;
			int32 TotalRooms = 0;
			GameInstance->GetStageProgress(Chapter, Room, TotalRooms);
			HUDWidget->SetStageInfo(Chapter, Room, TotalRooms);
		}

		bHUDInitialized = true;
	}

	// OnDeath도 한 번만 바인딩
	UURGAttributeComponent* Attribute = InPawn->FindComponentByClass<UURGAttributeComponent>();
	if (Attribute != nullptr && !bDeathBound)
	{
		Attribute->OnDeath.AddUObject(this, &AURGPlayerController::HandlePlayerDeath);
		bDeathBound = true;
	}
}

void AURGPlayerController::HandlePlayerDeath()
{
	bIsPlayerDead = true;

	// 게임오버 UI 조작을 위해 커서를 켜고 게임 입력을 막는다 (일시정지는 하지 않음 — 사망 연출이 멈추지 않도록)
	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());

	// 래그돌로 쓰러지는 장면을 먼저 보여 준 뒤 화면을 띄운다.
	// SetTimer는 0초를 넣으면 타이머를 지우기만 하므로 지연이 없으면 바로 호출한다.
	if (GameOverDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(GameOverTimer, this, &AURGPlayerController::ShowGameOver, GameOverDelay, false);
	}
	else
	{
		ShowGameOver();
	}

	OnPlayerDied.Broadcast();
}

void AURGPlayerController::ShowGameOver()
{
	ShowResultScreen(GameOverWidgetClass);
}

void AURGPlayerController::HandleRunCompleted()
{
	// 엔딩은 맵을 옮기지 않으므로, 출구 진입 때 건 이동 실패 감지가 화면을 다시 밝히지 않게 끈다.
	// 출구가 덮은 검은 화면은 그대로 엔딩 화면의 배경이 된다
	check(StageTransitionComponent);
	StageTransitionComponent->CancelTravelCover();

	bShowMouseCursor = true;
	ShowResultScreen(EndingWidgetClass);
}

void AURGPlayerController::ShowResultScreen(TSubclassOf<UURGGameOverUI> InWidgetClass)
{
	// 게임오버와 엔딩이 겹치지 않게, 먼저 뜬 결과 화면만 유지한다
	if (!IsLocalController() || ResultWidget != nullptr || InWidgetClass == nullptr)
	{
		return;
	}

	ResultWidget = CreateWidget<UURGGameOverUI>(this, InWidgetClass);
	if (ResultWidget == nullptr)
	{
		return;
	}

	ResultWidget->OnRetryRequested.AddUObject(this, &AURGPlayerController::HandleRetryRequested);
	ResultWidget->OnQuitRequested.AddUObject(this, &AURGPlayerController::HandleQuitRequested);
	ResultWidget->OnTitleRequested.AddUObject(this, &AURGPlayerController::HandleTitleRequested);

	if (const UURGGameInstance* GameInstance = GetGameInstance<UURGGameInstance>())
	{
		int32 Chapter = 0;
		int32 Room = 0;
		int32 TotalRooms = 0;
		GameInstance->GetStageProgress(Chapter, Room, TotalRooms);
		ResultWidget->SetProgress(Chapter, Room, TotalRooms);
	}

	// 마지막 기체 상태를 보여 준다 (사망 후에도 폰은 래그돌로 남아 있다)
	if (const APawn* OwnedPawn = GetPawn())
	{
		ResultWidget->SetPartSlots(OwnedPawn->FindComponentByClass<UURGPartComponent>());
	}

	ResultWidget->AddToViewport();

	// HUD보다 위에서 키보드·게임패드 입력도 받도록 위젯에 포커스를 준다
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(ResultWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AURGPlayerController::HandleRetryRequested()
{
	TravelFromResultScreen(&UURGGameInstance::RequestRestartRun);
}

void AURGPlayerController::HandleQuitRequested()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void AURGPlayerController::HandleTitleRequested()
{
	TravelFromResultScreen(&UURGGameInstance::RequestReturnToTitle);
}

void AURGPlayerController::TravelFromResultScreen(bool (UURGGameInstance::*InRequest)())
{
	// 입력 모드는 뷰포트에 남아 맵을 옮겨도 유지되므로, 이동 전에 게임 입력으로 되돌린다.
	// 도착한 맵의 컨트롤러(타이틀 포함)가 필요하면 다시 UI 입력으로 바꾼다
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());

	UURGGameInstance* GameInstance = GetGameInstance<UURGGameInstance>();
	if (GameInstance != nullptr && (GameInstance->*InRequest)())
	{
		return;
	}

	// 이동에 실패하면(스테이지 풀이 비어 있는 등) 화면에 갇히지 않게 결과 화면 조작을 되살린다
	RestoreResultScreenInput();
}

void AURGPlayerController::RestoreResultScreenInput()
{
	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());
	if (ResultWidget != nullptr)
	{
		ResultWidget->SetIsEnabled(true);
	}
}

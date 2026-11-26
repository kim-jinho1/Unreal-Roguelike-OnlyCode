#include "Component/URGStageTransitionComponent.h"
#include "Core/URGPlayerController.h"
#include "Core/URGStageExit.h"
#include "UI/URGPlayerHUD.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h"
#include "TimerManager.h"

UURGStageTransitionComponent::UURGStageTransitionComponent()
{
	// 도착·출구 진입 이벤트와 타이머로만 동작하므로 매 프레임 Tick은 필요하지 않다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UURGStageTransitionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerController = Cast<AURGPlayerController>(GetOwner());
	check(OwnerController);

	// 화면 연출은 이 기기에서 조작하는 컨트롤러만 한다
	if (!OwnerController->IsLocalController())
	{
		return;
	}

	// 출구는 맵에 미리 배치되어 BeginPlay 시점에 모두 존재한다
	for (TActorIterator<AURGStageExit> It(GetWorld()); It; ++It)
	{
		It->OnExitEntered.AddUniqueDynamic(this, &UURGStageTransitionComponent::HandleExitEntered);
	}

	// 맵 이동(OpenLevel)은 월드를 통째로 다시 만들어 첫 프레임들이 끊긴다. 검은 화면으로 시작해 그 구간을 가린다
	if (OwnerController->PlayerCameraManager != nullptr)
	{
		OwnerController->PlayerCameraManager->SetManualCameraFade(1.0f, FLinearColor::Black, false);
	}

	// SetTimer는 0초를 넣으면 타이머를 지우기만 하므로 유지 시간이 없으면 바로 밝힌다
	if (ArrivalFadeHoldDuration > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(ArrivalFadeTimer, this, &UURGStageTransitionComponent::FadeInFromBlack, ArrivalFadeHoldDuration, false);
	}
	else
	{
		FadeInFromBlack();
	}
}

void UURGStageTransitionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ArrivalFadeTimer);
		World->GetTimerManager().ClearTimer(TravelCoverTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void UURGStageTransitionComponent::CancelTravelCover()
{
	GetWorld()->GetTimerManager().ClearTimer(TravelCoverTimer);
}

void UURGStageTransitionComponent::HandleExitEntered(APawn* InPlayer)
{
	if (InPlayer != OwnerController->GetPawn() || OwnerController->PlayerCameraManager == nullptr)
	{
		return;
	}

	// 출구는 이 알림 직후 OpenLevel을 요청하고, 실제 로딩은 다음 프레임에 시작된다.
	// 이번 프레임을 검게 그려 두면 로딩 동안 멈춰 있는 화면이 검은 화면이 된다
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	TimerManager.ClearTimer(ArrivalFadeTimer);
	OwnerController->PlayerCameraManager->SetManualCameraFade(1.0f, FLinearColor::Black, false);

	// 카메라 페이드는 3D 화면만 덮으므로 HUD는 따로 숨긴다. 새 맵에서는 HUD가 새로 만들어진다
	UURGPlayerHUD* HUDWidget = OwnerController->GetHUDWidget();
	ESlateVisibility PreviousHUDVisibility = ESlateVisibility::SelfHitTestInvisible;
	if (HUDWidget != nullptr)
	{
		PreviousHUDVisibility = HUDWidget->GetVisibility();
		HUDWidget->SetVisibility(ESlateVisibility::Hidden);
	}

	// 맵이 바뀌면 이 월드와 함께 타이머도 사라진다. 남아 있다면 이동이 실패한 것이므로 HUD와 화면을 되살린다
	TimerManager.SetTimer(TravelCoverTimer, FTimerDelegate::CreateWeakLambda(this, [this, PreviousHUDVisibility]()
	{
		if (UURGPlayerHUD* HUD = OwnerController->GetHUDWidget())
		{
			HUD->SetVisibility(PreviousHUDVisibility);
		}

		FadeInFromBlack();
	}), TravelCoverTimeout, false);
}

void UURGStageTransitionComponent::FadeInFromBlack() const
{
	if (OwnerController->PlayerCameraManager == nullptr)
	{
		return;
	}

	OwnerController->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, FadeInDuration, FLinearColor::Black, false, false);
}

#include "Equipment/URGRepairStation.h"
#include "Blueprint/UserWidget.h"
#include "Component/URGAttributeComponent.h"
#include "Component/URGPartComponent.h"
#include "Component/URGResourceComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Equipment/URGPartDataAsset.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UI/URGMinigameUI.h"
#include "UI/URGRepairMenuUI.h"
#include "UI/URGWeldMinigameUI.h"
#include "UI/URGWireMinigameUI.h"
#include "UI/URGRAMMinigameUI.h"

AURGRepairStation::AURGRepairStation()
{
	PrimaryActorTick.bCanEverTick = false;
	LegMinigameClass = UURGWeldMinigameUI::StaticClass();
	CoreMinigameClass = UURGWireMinigameUI::StaticClass();
	HeadMinigameClass = UURGRAMMinigameUI::StaticClass();

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
	SetRootComponent(Mesh);

	// 상호작용 트레이스에만 걸린다
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_InteractTrace, ECR_Block);
}

void AURGRepairStation::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bIsSessionOpen)
	{
		CloseSession();
	}

	Super::EndPlay(EndPlayReason);
}

void AURGRepairStation::Interact_Implementation(AActor* InInstigator)
{
	APawn* Pawn = Cast<APawn>(InInstigator);
	APlayerController* Controller = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	UURGPartComponent* Part = InInstigator ? InInstigator->FindComponentByClass<UURGPartComponent>() : nullptr;

	if (Controller == nullptr || Part == nullptr || MenuClass == nullptr
		|| (MinigameClass == nullptr && LegMinigameClass == nullptr && CoreMinigameClass == nullptr && HeadMinigameClass == nullptr))
	{
		return;
	}

	Menu = CreateWidget<UURGRepairMenuUI>(Controller, MenuClass);
	if (Menu == nullptr)
	{
		return;
	}

	PlayerController = Controller;
	PartComponent = Part;
	ResourceComponent = InInstigator->FindComponentByClass<UURGResourceComponent>();
	bIsSessionOpen = true;

	// 세션 중 죽으면 위젯을 치워야 하므로 사망 통지를 받는다
	AttributeComponent = InInstigator->FindComponentByClass<UURGAttributeComponent>();
	if (AttributeComponent.IsValid())
	{
		DeathHandle = AttributeComponent->OnDeath.AddUObject(this, &AURGRepairStation::HandleInstigatorDeath);
	}

	Menu->OnSlotSelected.AddUObject(this, &AURGRepairStation::HandleSlotSelected);
	Menu->OnCancelled.AddUObject(this, &AURGRepairStation::HandleMenuCancelled);
	const int32 CurrentResource = ResourceComponent.IsValid() ? ResourceComponent->GetResource() : 0;
	Menu->InitializeForPart(Part, CurrentResource, RepairCost);
	Menu->AddToViewport();

	FocusWidget(Menu);
}

FText AURGRepairStation::GetInteractText_Implementation() const
{
	return InteractText;
}

bool AURGRepairStation::CanInteract_Implementation(AActor* InInstigator) const
{
	if (bIsSessionOpen || InInstigator == nullptr)
	{
		return false;
	}

	const UURGPartComponent* Part = InInstigator->FindComponentByClass<UURGPartComponent>();
	if (Part == nullptr)
	{
		return false;
	}

	// 자원이 모자라도 메뉴는 열어 비용과 부족분을 보여 준다 (막아 버리면 왜 수리가 안 되는지 알 수 없다). 차감은 슬롯 선택 시
	return Part->NeedsRepair(EURGPartSlot::Arm) || Part->NeedsRepair(EURGPartSlot::Leg)
		|| Part->NeedsRepair(EURGPartSlot::Core) || Part->NeedsRepair(EURGPartSlot::Head);
}

void AURGRepairStation::HandleSlotSelected(EURGPartSlot InSlot)
{
	APlayerController* Controller = PlayerController.Get();
	if (Controller == nullptr)
	{
		CloseSession();
		return;
	}

	TSubclassOf<UURGMinigameUI> SelectedClass = MinigameClass;
	if (InSlot == EURGPartSlot::Leg)
	{
		SelectedClass = LegMinigameClass.Get();
	}
	else if (InSlot == EURGPartSlot::Core)
	{
		SelectedClass = CoreMinigameClass.Get();
	}
	else if (InSlot == EURGPartSlot::Head)
	{
		SelectedClass = HeadMinigameClass.Get();
	}
	if (SelectedClass == nullptr || !PartComponent.IsValid() || !PartComponent->NeedsRepair(InSlot))
	{
		CloseSession();
		return;
	}

	if (RepairCost > 0 && (!ResourceComponent.IsValid() || !ResourceComponent->TrySpend(RepairCost)))
	{
		CloseSession();
		return;
	}

	TargetSlot = InSlot;

	if (Menu)
	{
		Menu->RemoveFromParent();
		Menu = nullptr;
	}

	Minigame = CreateWidget<UURGMinigameUI>(Controller, SelectedClass);
	if (Minigame == nullptr)
	{
		CloseSession();
		return;
	}

	Minigame->OnFinished.AddUObject(this, &AURGRepairStation::HandleMinigameFinished);
	Minigame->Start();

	if (Minigame && Minigame->IsRunning())
	{
		FocusWidget(Minigame);
	}
}

void AURGRepairStation::HandleMenuCancelled()
{
	CloseSession();
}

void AURGRepairStation::HandleMinigameFinished(float InScore)
{
	// 부분 회복은 없다. 기준 점수를 넘겼을 때만 최대치까지 수리
	const float SuccessScore = TargetSlot == EURGPartSlot::Head ? 1.0f
		: (TargetSlot == EURGPartSlot::Leg ? LegRequiredScore : RequiredScore);
	if (InScore >= SuccessScore)
	{
		if (PartComponent.IsValid())
		{
			const FURGPartSlotState& State = PartComponent->GetSlotState(TargetSlot);
			if (State.IsEquipped())
			{
				PartComponent->RepairSlot(TargetSlot, State.Part->GetMaxDurability());
			}
		}

		// 부품만 고치고 체력은 그대로면 정비 지점 역할을 못 한다. 게임에 다른 회복 수단이 없으므로 여기서 함께 회복한다.
		if (RepairHealAmount > 0.0f && AttributeComponent.IsValid())
		{
			AttributeComponent->Heal(RepairHealAmount);
		}
	}

	CloseSession();
}

void AURGRepairStation::HandleInstigatorDeath()
{
	// 입력 모드는 컨트롤러가 게임오버용으로 바꾸므로 여기서 되돌리면 안 된다
	CloseSession(false);
}

void AURGRepairStation::CloseSession(bool bInRestoreInput)
{
	if (Menu)
	{
		Menu->RemoveFromParent();
		Menu = nullptr;
	}

	if (Minigame)
	{
		Minigame->RemoveFromParent();
		Minigame = nullptr;
	}

	APlayerController* Controller = PlayerController.Get();
	if (bInRestoreInput && Controller != nullptr)
	{
		Controller->SetInputMode(FInputModeGameOnly());
		Controller->bShowMouseCursor = false;
	}

	if (AttributeComponent.IsValid())
	{
		AttributeComponent->OnDeath.Remove(DeathHandle);
	}
	DeathHandle.Reset();

	PlayerController = nullptr;
	PartComponent = nullptr;
	ResourceComponent = nullptr;
	AttributeComponent = nullptr;
	bIsSessionOpen = false;
}

void AURGRepairStation::FocusWidget(UUserWidget* InWidget) const
{
	APlayerController* Controller = PlayerController.Get();
	if (Controller == nullptr || InWidget == nullptr)
	{
		return;
	}

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(InWidget->TakeWidget());
	Controller->SetInputMode(InputMode);
	Controller->bShowMouseCursor = true;
}

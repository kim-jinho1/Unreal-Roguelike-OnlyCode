#include "Equipment/URGRepairStation.h"
#include "Component/URGAttributeComponent.h"
#include "Component/URGPartComponent.h"
#include "Component/URGResourceComponent.h"
#include "Core/URGPlayerController.h"
#include "Equipment/URGPartDataAsset.h"
#include "UI/URGMinigameUI.h"
#include "UI/URGRAMMinigameUI.h"
#include "UI/URGRepairMenuUI.h"
#include "UI/URGWeldMinigameUI.h"
#include "UI/URGWireMinigameUI.h"
#include "Blueprint/UserWidget.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"

AURGRepairStation::AURGRepairStation()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
	SetRootComponent(Mesh);

	// 상호작용 트레이스에만 걸린다
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_InteractTrace, ECR_Block);

	// 팔은 BP에서 볼트 미니게임 WBP를 넣는다. 나머지 셋은 WBP 없이 C++ 위젯이 직접 그리므로 클래스를 바로 쓴다
	SlotRules.Add(EURGPartSlot::Arm, FURGRepairRule());
	// 용접은 경로 이탈 거리로 점수를 매기므로 완벽을 요구하지 않는다
	SlotRules.Add(EURGPartSlot::Leg, FURGRepairRule(UURGWeldMinigameUI::StaticClass(), 0.75f));
	SlotRules.Add(EURGPartSlot::Core, FURGRepairRule(UURGWireMinigameUI::StaticClass(), 1.0f));
	// 램 장착은 성공(1)·실패(0)만 낸다
	SlotRules.Add(EURGPartSlot::Head, FURGRepairRule(UURGRAMMinigameUI::StaticClass(), 1.0f));
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
	const APawn* Pawn = Cast<APawn>(InInstigator);
	AURGPlayerController* Controller = Pawn != nullptr ? Cast<AURGPlayerController>(Pawn->GetController()) : nullptr;
	UURGPartComponent* Part = InInstigator != nullptr ? InInstigator->FindComponentByClass<UURGPartComponent>() : nullptr;

	if (Controller == nullptr || Part == nullptr || MenuClass == nullptr)
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

	Controller->OpenModalWidget(Menu);
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
	AURGPlayerController* Controller = PlayerController.Get();
	const FURGRepairRule* Rule = SlotRules.Find(InSlot);
	if (Controller == nullptr || Rule == nullptr || Rule->MinigameClass == nullptr
		|| !PartComponent.IsValid() || !PartComponent->NeedsRepair(InSlot))
	{
		CloseSession();
		return;
	}

	// 되돌릴 수 없는 비용 차감은 실패할 수 있는 위젯 생성 뒤에 한다
	UURGMinigameUI* NewMinigame = CreateWidget<UURGMinigameUI>(Controller, Rule->MinigameClass);
	if (NewMinigame == nullptr)
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
	TargetRequiredScore = Rule->RequiredScore;

	if (Menu != nullptr)
	{
		Menu->RemoveFromParent();
		Menu = nullptr;
	}

	Minigame = NewMinigame;
	Minigame->OnFinished.AddUObject(this, &AURGRepairStation::HandleMinigameFinished);
	Minigame->Start();

	// 미니게임은 Start 안에서 바로 끝날 수 있다 (용접 경로 데이터가 비었을 때 등). 그러면 HandleMinigameFinished가 이미 세션을 닫았다
	if (Minigame != nullptr && Minigame->IsRunning())
	{
		Controller->OpenModalWidget(Minigame);
	}
}

void AURGRepairStation::HandleMenuCancelled()
{
	CloseSession();
}

void AURGRepairStation::HandleMinigameFinished(float InScore)
{
	// 부분 회복은 없다. 슬롯 기준 점수를 넘겼을 때만 최대치까지 수리
	if (InScore >= TargetRequiredScore)
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
	// 컨트롤러도 같은 OnDeath로 게임오버 입력을 걸고, 사망 뒤의 모달 닫기는 무시한다. 여기서는 세션만 닫으면 된다
	CloseSession();
}

void AURGRepairStation::CloseSession()
{
	if (Menu != nullptr)
	{
		Menu->RemoveFromParent();
		Menu = nullptr;
	}

	if (Minigame != nullptr)
	{
		Minigame->RemoveFromParent();
		Minigame = nullptr;
	}

	if (AURGPlayerController* Controller = PlayerController.Get())
	{
		Controller->CloseModalWidget();
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

#include "Component/URGBurnComponent.h"
#include "Characters/URGCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UURGBurnComponent::UURGBurnComponent()
{
	// 피해는 타이머 간격으로만 주므로 매 프레임 Tick은 필요하지 않다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UURGBurnComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BurnTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void UURGBurnComponent::StartBurn(AActor* InCauser, const float InDamagePerTick, const float InInterval, const float InDuration)
{
	UWorld* World = GetWorld();
	if (World == nullptr || InInterval <= 0.0f)
	{
		return;
	}

	Causer = InCauser;
	DamagePerTick = InDamagePerTick;
	RemainingTicks = FMath::FloorToInt(InDuration / InInterval);

	// 이미 타는 중이면 간격을 다시 걸지 않는다. 연사로 계속 맞을 때 타이머가 매번 리셋되면 피해가 영영 안 들어간다
	if (!World->GetTimerManager().IsTimerActive(BurnTimer))
	{
		World->GetTimerManager().SetTimer(BurnTimer, this, &UURGBurnComponent::HandleBurnTick, InInterval, true);
	}
}

void UURGBurnComponent::HandleBurnTick()
{
	AActor* OwnerActor = GetOwner();
	const AURGCharacter* OwnerCharacter = Cast<AURGCharacter>(OwnerActor);
	if (OwnerActor == nullptr || RemainingTicks <= 0 || (OwnerCharacter && OwnerCharacter->IsDead()))
	{
		StopBurn();
		return;
	}

	--RemainingTicks;

	AActor* CauserActor = Causer.Get();
	const APawn* CauserPawn = Cast<APawn>(CauserActor);
	UGameplayStatics::ApplyDamage(OwnerActor, DamagePerTick, CauserPawn ? CauserPawn->GetController() : nullptr, CauserActor, nullptr);

	// 이번 피해로 죽었거나 횟수를 다 썼으면 다음 간격을 기다리지 않고 정리한다
	if (RemainingTicks <= 0 || (OwnerCharacter && OwnerCharacter->IsDead()))
	{
		StopBurn();
	}
}

void UURGBurnComponent::StopBurn()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BurnTimer);
	}

	DestroyComponent();
}


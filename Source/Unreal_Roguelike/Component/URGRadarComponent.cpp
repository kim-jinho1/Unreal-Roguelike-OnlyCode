#include "Component/URGRadarComponent.h"
#include "Characters/URGCharacter.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "TimerManager.h"

UURGRadarComponent::UURGRadarComponent()
{
	// 탐지는 타이머 간격으로만 하므로 매 프레임 Tick은 필요하지 않다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UURGRadarComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScanTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void UURGRadarComponent::SetRadarEnabled(const bool bInEnabled)
{
	UWorld* World = GetWorld();
	if (World == nullptr || bRadarEnabled == bInEnabled)
	{
		return;
	}

	bRadarEnabled = bInEnabled;

	if (bRadarEnabled)
	{
		World->GetTimerManager().SetTimer(ScanTimer, this, &UURGRadarComponent::Scan, ScanInterval, true);
		Scan();
	}
	else
	{
		World->GetTimerManager().ClearTimer(ScanTimer);
		OnRadarUpdated.Broadcast(TArray<FVector>());
	}
}

void UURGRadarComponent::Scan()
{
	UWorld* World = GetWorld();
	AActor* OwnerActor = GetOwner();
	if (World == nullptr || OwnerActor == nullptr)
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(URGRadarScan), false, OwnerActor);

	// 적 콜리전 프로필에 기대지 않도록 동적 오브젝트 전부를 훑고 캐릭터만 남긴다
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, OwnerActor->GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllDynamicObjects),
		FCollisionShape::MakeSphere(RadarRange), QueryParams);

	// 한 캐릭터가 캡슐·메시 등 여러 컴포넌트로 잡히므로 액터 단위로 모은다
	TArray<const AActor*> Found;
	TArray<FVector> Locations;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		const AURGCharacter* Character = Cast<AURGCharacter>(Overlap.GetActor());
		if (Character == nullptr || Character == OwnerActor || Character->IsDead() || Found.Contains(Character))
		{
			continue;
		}

		Found.Add(Character);
		Locations.Add(Character->GetActorLocation());
	}

	OnRadarUpdated.Broadcast(Locations);
}


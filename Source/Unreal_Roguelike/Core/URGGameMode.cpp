#include "Core/URGGameMode.h"
#include "Engine/World.h"
#include "Equipment/URGResourcePickup.h"

void AURGGameMode::SpawnResourceDrop(const FVector& InLocation) const
{
	UWorld* World = GetWorld();
	if (ResourcePickupClass == nullptr || World == nullptr)
	{
		return;
	}

	// 죽은 적의 몸 중심에 스폰하면 픽업이 스스로 바닥으로 내려앉는다
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<AURGResourcePickup>(ResourcePickupClass, InLocation, FRotator::ZeroRotator, SpawnParams);
}

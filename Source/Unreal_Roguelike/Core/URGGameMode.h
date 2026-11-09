#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "URGGameMode.generated.h"

/**
 * 스테이지 게임모드. 적 사망 시 떨어지는 자원 픽업 스폰을 맡는다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API AURGGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/** 적이 죽은 위치에 자원 픽업을 떨어뜨린다 (ResourcePickupClass가 없으면 무시) */
	void SpawnResourceDrop(const FVector& InLocation) const;

protected:
	/** 적이 죽으면 떨어지는 자원 픽업 */
	UPROPERTY(EditDefaultsOnly, Category = "GameMode|Drop")
	TSubclassOf<class AURGResourcePickup> ResourcePickupClass;

};

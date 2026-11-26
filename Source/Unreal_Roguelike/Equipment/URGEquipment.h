#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "URGEquipment.generated.h"

/**
 * 장착 가능한 장비 액터의 베이스.
 * 외형은 소유 캐릭터 메시에 포함돼 있어 자체 메시 없이 빈 루트만 두고, 소유 캐릭터의 메시 소켓에 부착된다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API AURGEquipment : public AActor
{
	GENERATED_BODY()

public:
	AURGEquipment();

public:
	/** 장착 처리. 파생 클래스가 소유자 컴포넌트 등록/부착을 수행한다 */
	virtual void EquipItem();

	/** 장착 해제 처리 */
	virtual void UnequipItem();

	/** 소유 캐릭터 메시의 소켓에 부착 (위치/회전 스냅, 스케일은 상대값 유지) */
	virtual void AttachToOwner(FName InSocketName);

};

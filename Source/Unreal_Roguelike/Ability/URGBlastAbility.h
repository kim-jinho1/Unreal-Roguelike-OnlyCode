#pragma once

#include "CoreMinimal.h"
#include "Ability/URGCoreAbility.h"
#include "URGBlastAbility.generated.h"

/**
 * 범위 폭발: 소유자 위치를 중심으로 반경 안의 액터에 방사형 피해를 준다. 즉발(Duration 0).
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGBlastAbility : public UURGCoreAbility
{
	GENERATED_BODY()

protected:
	virtual void Activate() override;

protected:
	/** 폭발 피해량 */
	UPROPERTY(EditDefaultsOnly, Category = "CoreAbility|Blast", meta = (ClampMin = "0"))
	float Damage = 80.0f;

	/** 폭발 반경 (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "CoreAbility|Blast", meta = (ClampMin = "0"))
	float Radius = 500.0f;

	/** 피해 타입 (없으면 기본 UDamageType) */
	UPROPERTY(EditDefaultsOnly, Category = "CoreAbility|Blast")
	TSubclassOf<class UDamageType> DamageTypeClass;

};

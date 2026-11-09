#pragma once

#include "CoreMinimal.h"
#include "Ability/URGCoreAbility.h"
#include "URGShieldAbility.generated.h"

/**
 * 방어 실드: 지속 시간 동안 받는 피해를 DamageScale 배율로 줄인다 (0이면 무적).
 * 지속 중 소유자를 감싸는 구체 메시를 붙여 배리어를 보여 준다 (메시·머티리얼은 BP에서 지정).
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGShieldAbility : public UURGCoreAbility
{
	GENERATED_BODY()

public:
	UURGShieldAbility();

protected:
	virtual void Activate() override;
	virtual void Deactivate() override;

protected:
	/** 실드 중 받는 피해 배율 (0 = 무적) */
	UPROPERTY(EditDefaultsOnly, Category = "CoreAbility|Shield", meta = (ClampMin = "0", ClampMax = "1"))
	float DamageScale = 0.0f;

	/** 소유자를 감쌀 구체 메시 (없으면 시각 효과 생략) */
	UPROPERTY(EditDefaultsOnly, Category = "CoreAbility|Shield")
	TObjectPtr<class UStaticMesh> ShieldMesh;

	/** 구체에 씌울 머티리얼 (없으면 메시 기본) */
	UPROPERTY(EditDefaultsOnly, Category = "CoreAbility|Shield")
	TObjectPtr<class UMaterialInterface> ShieldMaterial;

	/** 구체 스케일 (엔진 기본 Sphere는 지름 100cm) */
	UPROPERTY(EditDefaultsOnly, Category = "CoreAbility|Shield", meta = (ClampMin = "0.1"))
	float ShieldScale = 2.2f;

private:
	/** 지속 중에만 존재하는 구체 컴포넌트 (Deactivate에서 파괴) */
	UPROPERTY()
	TObjectPtr<class UStaticMeshComponent> ShieldComponent;

};

#pragma once

#include "CoreMinimal.h"
#include "Equipment/URGEquipment.h"
#include "GameplayTagContainer.h"
#include "URGWeapon.generated.h"

/**
 * 무기 베이스 (원거리/근접 공통).
 * 스폰·등록·제거는 소유자의 CombatComponent가 담당하고, 무기는 장착 시 캐릭터 메시 소켓에 부착된다.
 * 공격 몽타주와 데미지 계산(공격 타입 배율 × 팔 부품 손상 배율)을 담당하며, 실제 판정은 파생 클래스가 Attack()에서 구현한다.
 * Howitzer처럼 캐릭터 메시에 포가 달린 경우 무기 메시는 비워 두고 소켓은 캐릭터 메시에서 가져온다.
 */
UCLASS(Abstract)
class UNREAL_ROGUELIKE_API AURGWeapon : public AURGEquipment
{
	GENERATED_BODY()

public:
	AURGWeapon();

public:
	virtual void EquipItem() override;
	virtual void UnequipItem() override;

public:
	/** 공격 수행. 파생 클래스가 판정을 구현한다 */
	virtual void Attack(FGameplayTag InAttackType);

	/** 무기 자체 조건으로 지금 공격할 수 있는지 (탄약 등). 캐릭터 상태는 캐릭터가 검사한다 */
	virtual bool CanAttack() const;

	/** 재장전 가능 여부 (탄약을 쓰는 무기만 true) */
	virtual bool CanReload() const;

	/** 재장전 (탄약을 쓰는 무기만 구현) */
	virtual void Reload();

	/** 이번 공격의 데미지를 굴린다 (연사는 RapidDamageScale, 팔 부품 배율·치명타는 CombatComponent). 호출마다 값이 다를 수 있다. 치명타였는지는 bOutCrit */
	float RollAttackDamage(bool& bOutCrit) const;

	FORCEINLINE class UAnimMontage* GetAttackMontage() const { return AttackMontage; }
	FORCEINLINE float GetAttackCooldown() const { return AttackCooldown; }

	/** 재장전 몽타주 (탄약을 쓰는 무기만 제공, 없으면 nullptr → 캐릭터가 타이머로 대기) */
	virtual class UAnimMontage* GetReloadMontage() const;
	FORCEINLINE class UURGCombatComponent* GetCombatComponent() const { return CombatComponent; }

	/** 소켓 월드 위치 (무기 메시 소켓 → 소유 캐릭터 메시 소켓 → 액터 위치 순으로 폴백) */
	FVector GetSocketLocationOrOwner(FName InSocketName) const;

protected:
	/** 소유자의 시점(컨트롤러 카메라, 없으면 소유 액터 위치·회전) */
	void GetOwnerViewPoint(FVector& OutLocation, FRotator& OutRotation) const;

	/**
	 * 명중 결과에 데미지 적용 (파생 클래스 공통). InDamage가 0 이하이면 현재 공격 타입의 데미지를 새로 굴린다 (이때 bInCrit은 무시)
	 * @return 살아 있는 캐릭터의 체력이 실제로 깎였는지 (벽·소품·시체면 false)
	 */
	bool ApplyDamageToHit(const FHitResult& InHit, const FVector& InDamageDirection, float InDamage = 0.0f, bool bInCrit = false);

	/** 살아 있는 캐릭터의 현재 체력. 캐릭터가 아니거나 이미 죽었으면 -1 (데미지 적용 전에 기록해 두고 ReportHit에 넘긴다) */
	static float GetAliveHealth(const AActor* InTarget);

	/**
	 * 데미지 적용 전후 체력을 비교해 CombatComponent에 적중을 알린다.
	 * 데미지 함수의 반환값은 요청한 값이라 방어·사망으로 실제 깎인 양과 다를 수 있어 체력 차이로 잰다
	 * @return 체력이 실제로 깎였는지
	 */
	bool ReportHit(const AActor* InTarget, float InHealthBefore, const FVector& InLocation, bool bInCrit) const;

	/** 소유자가 플레이어일 때만 컨트롤러를 돌려준다 (적이 같은 무기를 써도 카메라 연출이 걸리지 않도록) */
	class APlayerController* GetOwnerPlayerController() const;

	/** 소유 플레이어 카메라를 흔든다. 적이 들고 있거나 배율이 0 이하면 아무것도 하지 않는다 */
	void PlayOwnerCameraShake(float InScale) const;

protected:
	/** 기본 공격력 */
	UPROPERTY(EditAnywhere, Category = "Weapon|Attack")
	float BaseDamage = 15.0f;

	/** 연사(Rapid) 공격 시 데미지 배율 */
	UPROPERTY(EditAnywhere, Category = "Weapon|Attack")
	float RapidDamageScale = 0.6f;

	/** 공격 시작부터 다음 공격까지 최소 간격 (초). 단발·연사 모두 적용. 0이면 공격 모션과 연사 최소 간격만으로 제한 */
	UPROPERTY(EditAnywhere, Category = "Weapon|Attack", meta = (ClampMin = "0"))
	float AttackCooldown = 0.0f;

	/** 공격 시 캐릭터가 재생할 몽타주 (근접은 이 몽타주의 노티파이 구간에서 판정한다) */
	UPROPERTY(EditAnywhere, Category = "Weapon|Attack")
	TObjectPtr<class UAnimMontage> AttackMontage;

	/** 장착 소켓 이름 (소유 캐릭터 메시) */
	UPROPERTY(EditAnywhere, Category = "Weapon|Socket")
	FName EquipSocketName = TEXT("WeaponAttachPoint");

	/** 발사·적중 시 재생할 카메라 셰이크 (세기는 파생 클래스의 배율 값) */
	UPROPERTY(EditAnywhere, Category = "Weapon|Feedback")
	TSubclassOf<class UCameraShakeBase> CameraShakeClass;

protected:
	/** 소유자의 전투 컴포넌트 (EquipItem 시점에 캐싱) */
	UPROPERTY()
	TObjectPtr<class UURGCombatComponent> CombatComponent;

	/** 현재 진행 중인 공격 타입 (데미지 계산용) */
	FGameplayTag CurrentAttackType;

};


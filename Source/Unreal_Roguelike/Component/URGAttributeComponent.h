#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "URGAttributeComponent.generated.h"

/** 체력 변경 알림 (CurrentHealth, MaxHealth) */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDelegateOnHealthChanged, float, float);

/** 데미지 받음 알림 (Damage, DamageCauser) */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDelegateOnDamaged, float, AActor*);

/** 사망 알림 */
DECLARE_MULTICAST_DELEGATE(FDelegateOnDeath);

/**
 * 체력 관리 컴포넌트.
 * 소유 액터의 OnTakeAnyDamage에 바인딩해 엔진 데미지 파이프라인으로만 체력이 깎인다.
 * 플레이어·적이 공통으로 사용하며, HUD는 델리게이트로만 값을 받는다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_ROGUELIKE_API UURGAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UURGAttributeComponent();

protected:
	virtual void BeginPlay() override;

public:
	FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	/** 생존 여부 */
	bool IsAlive() const;

	/** 마지막으로 데미지를 준 액터 (사망 연출의 밀침 방향 계산용). 없거나 파괴됐으면 nullptr */
	FORCEINLINE AActor* GetLastDamageCauser() const { return LastDamageCauser.Get(); }

	/** 체력 회복 */
	void Heal(float Amount);

	/** 최대 체력 증가 (증가분만큼 현재 체력도 함께 회복) */
	void AddMaxHealth(float Amount);

	/** 최대 체력을 직접 지정하고 현재 체력을 가득 채운다 (스폰 전 초기화용) */
	void SetMaxHealth(float NewMaxHealth);

	/** 저장해 둔 체력을 그대로 되돌린다 (스테이지 이동 복원용). 최대 체력도 함께 맞춘다 */
	void RestoreHealth(float InCurrentHealth, float InMaxHealth);

public:
	/** 체력이 변경될 때 호출 */
	FDelegateOnHealthChanged OnHealthChanged;

	/** 체력이 0이 되어 사망할 때 호출 */
	FDelegateOnDeath OnDeath;

	/** 데미지를 받고도 살아 있을 때 호출 (Damage, DamageCauser). 사망 시에는 OnDeath만 호출 */
	FDelegateOnDamaged OnDamaged;

protected:
	/** Owner가 데미지를 받을 때 호출 (OnTakeAnyDamage에 바인딩) */
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

protected:
	/** 최대 체력 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute|Health")
	float MaxHealth = 100.0f;

	/** 현재 체력 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute|Health")
	float CurrentHealth = 0.0f;

	/** 사망 상태 플래그 (OnDeath 중복 방송 방지) */
	bool bIsDead = false;

	/** 마지막 데미지의 가해자 (HandleTakeAnyDamage에서 갱신) */
	TWeakObjectPtr<AActor> LastDamageCauser;

};

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "URGCombatComponent.generated.h"

class AURGWeapon;

/** 주 무기 교체 알림 */
DECLARE_MULTICAST_DELEGATE(FDelegateOnWeaponChanged);

/** 무기 적중 알림 (적중 위치, 실제로 깎인 체력, 치명타 여부, 이번 적중으로 처치했는지) */
DECLARE_MULTICAST_DELEGATE_FourParams(FDelegateOnHitDealt, const FVector&, float, bool, bool);

/**
 * 캐릭터의 전투 상태(주 무기, 공격력 배율, 치명타)를 관리한다.
 * EquipWeapon()/UnequipWeapon()이 무기 액터의 스폰·등록·제거를 모두 담당하고, 무기는 EquipItem()에서 부착만 한다.
 * 팔 부품이 바뀌면 캐릭터가 EquipWeapon()과 SetAttackDamageScale()을, 머리 부품이 바뀌면 SetCritStats()를 호출한다.
 * 무기는 데미지를 낼 때 ModifyOutgoingDamage()로 배율과 치명타를 한 번에 적용하고,
 * 데미지를 준 뒤 ApplyOnHitEffects()로 태그 시너지 적중 효과(화상·전이)를 건다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_ROGUELIKE_API UURGCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UURGCombatComponent();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	FORCEINLINE AURGWeapon* GetMainWeapon() const { return MainWeapon; }

	/** 기존 주 무기를 제거하고 새 무기 클래스를 스폰해 장착한다. nullptr이면 해제만 한다 */
	void EquipWeapon(TSubclassOf<AURGWeapon> InWeaponClass);

	/** 주 무기 해제 및 액터 제거 */
	void UnequipWeapon();

	FORCEINLINE bool IsCombatEnabled() const { return bIsCombatEnabled; }
	FORCEINLINE void SetCombatEnabled(const bool bInEnabled) { bIsCombatEnabled = bInEnabled; }

	FORCEINLINE float GetAttackDamageScale() const { return AttackDamageScale; }
	FORCEINLINE void SetAttackDamageScale(const float InScale) { AttackDamageScale = InScale; }

	/** 치명타 확률·배율 지정 (머리 부품 반영) */
	void SetCritStats(float InChance, float InDamageScale);

	/** 무기 기본 데미지에 공격력 배율을 곱하고 치명타를 굴린다. 무기가 데미지를 낼 때마다 호출. 치명타였는지는 bOutCrit으로 돌려준다 */
	float ModifyOutgoingDamage(float InDamage, bool& bOutCrit) const;

	/** 무기가 살아 있는 캐릭터의 체력을 깎았을 때 호출. HUD(히트 마커·데미지 숫자)에 알린다 */
	void NotifyHitDealt(const FVector& InLocation, float InDamage, bool bInCrit, bool bInKilled) const;

	/** 시너지 수치 (HUD 설명 문구용) */
	FORCEINLINE float GetBurnDamagePerTick() const { return BurnDamagePerTick; }
	FORCEINLINE float GetBurnTickInterval() const { return BurnTickInterval; }
	FORCEINLINE float GetBurnDuration() const { return BurnDuration; }
	FORCEINLINE float GetChainRadius() const { return ChainRadius; }
	FORCEINLINE int32 GetChainMaxTargets() const { return ChainMaxTargets; }
	FORCEINLINE float GetChainDamageRatio() const { return ChainDamageRatio; }

	/** 태그 시너지가 켠 적중 효과 지정 (열 = 화상, 전기 = 주변 전이) */
	void SetSynergyEffects(bool bInBurn, bool bInChain);

	/** 무기가 대상에게 데미지를 준 직후 호출. 살아 있는 캐릭터에게만 화상·전이를 건다 */
	void ApplyOnHitEffects(AActor* InTarget, const FVector& InHitLocation, float InDamage) const;

public:
	/** 주 무기 변경 시 호출되는 델리게이트 (HUD의 탄약 재구독용) */
	FDelegateOnWeaponChanged OnWeaponChanged;

	/** 무기 적중 시 호출되는 델리게이트 (HUD 타격 피드백용). 화상·전이 피해는 무기를 거치지 않아 알리지 않는다 */
	FDelegateOnHitDealt OnHitDealt;

protected:
	/** 현재 장착 중인 주 무기 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	TObjectPtr<AURGWeapon> MainWeapon;

	/** 무기 데미지에 곱해지는 배율 (팔 부품 손상 단계가 반영된다) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	float AttackDamageScale = 1.0f;

	/** 치명타 확률 0~1 (머리 부품이 반영된다) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Crit")
	float CritChance = 0.0f;

	/** 치명타 시 데미지 배율 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Crit")
	float CritDamageScale = 1.0f;

	/** 전투(발사) 활성화 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|State")
	bool bIsCombatEnabled = true;

	/** 열 시너지: 화상 한 번에 주는 피해 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Synergy", meta = (ClampMin = "0"))
	float BurnDamagePerTick = 5.0f;

	/** 열 시너지: 화상 피해 간격 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Synergy", meta = (ClampMin = "0.05"))
	float BurnTickInterval = 0.5f;

	/** 열 시너지: 화상 지속 시간 (초). 다시 맞으면 처음부터 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Synergy", meta = (ClampMin = "0"))
	float BurnDuration = 3.0f;

	/** 전기 시너지: 맞은 적 주변에서 전이 대상을 찾는 반경 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Synergy", meta = (ClampMin = "0"))
	float ChainRadius = 400.0f;

	/** 전기 시너지: 최대 전이 대상 수 (가까운 순) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Synergy", meta = (ClampMin = "0"))
	int32 ChainMaxTargets = 2;

	/** 전기 시너지: 전이 피해 = 적중 피해 × 이 비율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Synergy", meta = (ClampMin = "0"))
	float ChainDamageRatio = 0.5f;

	/** 열 시너지 발동 중 (캐릭터가 SetSynergyEffects로 갱신) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Synergy")
	bool bBurnEnabled = false;

	/** 전기 시너지 발동 중 (캐릭터가 SetSynergyEffects로 갱신) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Synergy")
	bool bChainEnabled = false;

private:
	/** 주 무기 설정 후 OnWeaponChanged 브로드캐스트 (nullptr = 해제) */
	void SetMainWeapon(AURGWeapon* InNewWeapon);

	/** 대상에 화상 컴포넌트를 붙이거나 이미 있으면 지속 시간을 갱신한다 */
	void ApplyBurn(AActor* InTarget) const;

	/** 대상 주변의 살아 있는 다른 캐릭터에게 피해를 옮긴다. 전이 피해는 무기를 거치지 않아 다시 전이되지 않는다 */
	void ApplyChain(const AActor* InTarget, const FVector& InCenter, float InDamage) const;

};


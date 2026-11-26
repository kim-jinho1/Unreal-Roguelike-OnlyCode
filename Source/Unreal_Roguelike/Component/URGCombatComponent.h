#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "URGCombatComponent.generated.h"

class AURGWeapon;

/** 주 무기 교체 알림 */
DECLARE_MULTICAST_DELEGATE(FDelegateOnWeaponChanged);

/** 무기 적중 알림 (적중 위치, 실제로 깎인 체력, 치명타 여부, 이번 적중으로 처치했는지) */
DECLARE_MULTICAST_DELEGATE_FourParams(FDelegateOnHitDealt, const FVector&, float, bool, bool);

/** 공격 쿨다운 변경 알림 (CooldownEndTime 쿨다운이 끝나는 월드 초, CooldownDuration 전체 쿨다운 초). 표시할 쿨다운이 없으면 둘 다 0 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDelegateOnAttackCooldownChanged, float, float);

/** 재장전이 끝났음을 알림 (bCompleted 탄창을 채웠으면 true, 취소됐으면 false) */
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateOnReloadEnded, bool);

/**
 * 캐릭터의 전투(주 무기, 공격·재장전 진행, 공격력 배율, 치명타)를 관리한다.
 * EquipWeapon()/UnequipWeapon()이 무기 액터의 스폰·등록·제거를 모두 담당하고, 무기는 EquipItem()에서 부착만 한다.
 * 공격 간격·공격 모션 대기·재장전 타이머와 몽타주 재생도 여기서 한다. 행동해도 되는 상태인지와 상태 태그는 소유 캐릭터가 정하고,
 * 이 컴포넌트는 재장전이 끝났다는 사실만 OnReloadEnded로 알린다.
 * 팔 부품이 바뀌면 캐릭터가 EquipWeapon()과 SetAttackDamageScale()을, 머리 부품이 바뀌면 SetCritStats()를 호출한다.
 * 무기는 데미지를 낼 때 ModifyOutgoingDamage()로 배율과 치명타를 한 번에 적용하고,
 * 데미지를 준 뒤 ApplyOnHitEffects()로 태그 시너지 적중 효과(화상·전이)를 건다.
 * 소유자는 ACharacter여야 한다 (공격·재장전 몽타주를 재생한다).
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_ROGUELIKE_API UURGCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UURGCombatComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	FORCEINLINE AURGWeapon* GetMainWeapon() const { return MainWeapon; }

	/** 진행 중인 재장전을 취소하고 기존 주 무기를 제거한 뒤 새 무기 클래스를 스폰해 장착한다. nullptr이면 해제만 한다. 끝나면 공격 쿨다운을 다시 알린다 */
	void EquipWeapon(TSubclassOf<AURGWeapon> InWeaponClass);

	/** 주 무기 해제 및 액터 제거 */
	void UnequipWeapon();

	/** 주 무기가 탄약이 없어 공격할 수 없는지. true면 호출자가 공격 대신 재장전을 시작한다 */
	bool NeedsReload() const;

	/** 공격: 공격 모션·공격 간격을 검사한 뒤 무기 공격, 공격 몽타주 재생, 쿨다운 알림. 캐릭터 상태·탄약 확인은 호출자 몫 */
	void TryAttack();

	/** 주 무기를 재장전할 수 있는지 (탄약을 쓰고 탄창이 덜 찼는지). 캐릭터 상태 확인은 호출자 몫 */
	bool CanReload() const;

	/** 재장전 시작: 재장전 몽타주(없으면 ReloadDuration)만큼 기다린 뒤 탄창을 채우고 OnReloadEnded(true). CanReload 확인은 호출자 몫 */
	void StartReload();

	/** 진행 중인 재장전 취소: 타이머·몽타주를 끊고 OnReloadEnded(false). 재장전 중이 아니면 아무 일도 하지 않는다 */
	void CancelReload();

	FORCEINLINE bool IsCombatEnabled() const { return bIsCombatEnabled; }
	FORCEINLINE void SetCombatEnabled(bool bInEnabled) { bIsCombatEnabled = bInEnabled; }

	FORCEINLINE float GetAttackDamageScale() const { return AttackDamageScale; }
	FORCEINLINE void SetAttackDamageScale(float InScale) { AttackDamageScale = InScale; }

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

	/** 공격·무기 교체 시 호출 (HUD 팔 슬롯 쿨다운 표시용) */
	FDelegateOnAttackCooldownChanged OnAttackCooldownChanged;

	/** 재장전이 끝나거나 취소될 때 호출 (소유 캐릭터가 일반행동 상태를 푼다) */
	FDelegateOnReloadEnded OnReloadEnded;

protected:
	/** 주 무기의 공격 몽타주가 아직 본 구간을 재생 중인지 (블렌드 아웃에 들어가면 false → 다음 공격 허용) */
	bool IsAttackMontagePlaying() const;

	/** 현재 무기의 공격 쿨다운을 마지막 공격 시각 기준으로 알린다. 표시 기준 미만이거나 아직 공격 전이면 표시를 지운다 */
	void BroadcastAttackCooldown() const;

	/** 재장전 대기 종료 (타이머 콜백): 무기 탄창을 채우고 OnReloadEnded(true) */
	void HandleReloadTimerExpired();

protected:
	/** 연사 최소 간격 (초). 무기 공격 쿨다운이 더 길면 그쪽을 따른다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Attack", meta = (ClampMin = "0"))
	float RapidFireInterval = 0.12f;

	/** 무기 공격 쿨다운이 이 값(초) 이상일 때만 HUD에 표시한다 (짧은 간격까지 띄우면 슬롯이 계속 깜빡인다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Attack", meta = (ClampMin = "0"))
	float AttackCooldownDisplayMin = 0.5f;

	/** 재장전 소요 시간 (초). 무기에 재장전 몽타주가 없을 때 사용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Reload", meta = (ClampMin = "0"))
	float ReloadDuration = 1.5f;

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
	/** 새 무기 클래스를 소유자 기준으로 스폰해 주 무기로 등록하고 장착한다 */
	void SpawnMainWeapon(TSubclassOf<AURGWeapon> InWeaponClass);

	/** 주 무기 설정 후 OnWeaponChanged 브로드캐스트 (nullptr = 해제) */
	void SetMainWeapon(AURGWeapon* InNewWeapon);

	/** 대상에 화상 컴포넌트를 붙이거나 이미 있으면 지속 시간을 갱신한다 */
	void ApplyBurn(AActor* InTarget) const;

	/** 대상 주변의 살아 있는 다른 캐릭터에게 피해를 옮긴다. 전이 피해는 무기를 거치지 않아 다시 전이되지 않는다 */
	void ApplyChain(const AActor* InTarget, const FVector& InCenter, float InDamage) const;

private:
	/** 소유 캐릭터 (BeginPlay에서 한 번 캐싱. 몽타주 재생용) */
	UPROPERTY()
	TObjectPtr<class ACharacter> OwnerCharacter;

	/** 마지막 공격 시각. 무기가 아니라 여기에 두어 무기를 바꿔도 공격 간격을 건너뛸 수 없다 */
	float LastFireTime = -1.0f;

	/** 재장전 대기 타이머. 살아 있으면 재장전 중이다 */
	FTimerHandle ReloadTimer;

};


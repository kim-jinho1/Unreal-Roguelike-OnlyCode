#pragma once

#include "CoreMinimal.h"
#include "Characters/URGCharacter.h"
#include "GameplayTagContainer.h"
#include "URGDefine.h"
#include "URGPlayerCharacter.generated.h"

struct FInputActionValue;
struct FURGPartSlotState;

/** 코어 능력 발동 알림 (HUD 문구용). 인자는 능력이 정한 메시지 */
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateOnCoreAbilityActivated, const FText&);

/** 코어 능력 상태 변경 알림 (bActive 지속 중 여부, CooldownEndTime 쿨다운이 끝나는 월드 초, CooldownDuration 전체 쿨다운 초). 쿨다운이 없으면 둘 다 0 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FDelegateOnCoreAbilityStateChanged, bool, float, float);

/** 대시 쿨다운 변경 알림 (CooldownEndTime 쿨다운이 끝나는 월드 초, CooldownDuration 전체 쿨다운 초). 대시를 못 쓰게 되면 둘 다 0 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDelegateOnDashCooldownChanged, float, float);

/** 공격 쿨다운 변경 알림 (CooldownEndTime 쿨다운이 끝나는 월드 초, CooldownDuration 전체 쿨다운 초). 표시할 쿨다운이 없으면 둘 다 0 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDelegateOnAttackCooldownChanged, float, float);

/** 태그 시너지 발동·해제 알림 (SynergyTag, bActive) */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDelegateOnSynergyChanged, const FGameplayTag&, bool);

/** 바라보는 상호작용 대상 변경 알림 (새 대상, 없으면 nullptr) */
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateOnInteractFocusChanged, AActor*);

/**
 * 플레이어 캐릭터 (회수 유닛).
 * 3인칭 카메라, 이동/점프/대시, 조준, 공격/재장전, 상호작용을 담당한다.
 * 입력 바인딩은 PlayerInputComponent, 조준 카메라 연출은 AimComponent, 무기 보유는 CombatComponent,
 * 보유 자원은 ResourceComponent, 부품 슬롯 상태는 PartComponent에 위임한다.
 * 부품 슬롯 변경(OnSlotChanged)을 받아 팔 부품 → 무기 교체·공격력 배율, 다리 부품 → 이동·대시 배율·점프 횟수,
 * 코어 부품 → Q 액티브 능력 인스턴스, 머리 부품 → 치명타(CombatComponent)·드랍률로 연결한다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API AURGPlayerCharacter : public AURGCharacter
{
	GENERATED_BODY()

public:
	AURGPlayerCharacter();

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* InPlayerInputComponent) override;

	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:
	FORCEINLINE class UURGPlayerInputComponent* GetPlayerInputComponent() const { return PlayerInputComponent; }
	FORCEINLINE class UURGAimComponent* GetAimComponent() const { return AimComponent; }
	FORCEINLINE class UCameraComponent* GetCamera() const { return Camera; }
	FORCEINLINE class UURGCombatComponent* GetCombatComponent() const { return CombatComponent; }
	FORCEINLINE class UURGResourceComponent* GetResourceComponent() const { return ResourceComponent; }
	FORCEINLINE class UURGPartComponent* GetPartComponent() const { return PartComponent; }
	FORCEINLINE class UURGRadarComponent* GetRadarComponent() const { return RadarComponent; }

	/** 현재 장착 중인 주 무기 (없으면 nullptr) */
	class AURGWeapon* GetMainWeapon() const;

	/** 조준 중인지 여부 (AimComponent에 위임) */
	bool IsAiming() const;

	/** 대시 가능 상태인지 검사 (행동 차단 상태 + 다리 부품 사용 가능 + 쿨다운) */
	bool CanDash() const;

	/** 공격 가능 상태인지 검사 (행동 차단 상태 + 전투 활성화 + 무기 보유). 팔을 못 쓰면 기본 근접 무기를 든다 */
	bool CanAttack() const;

	/** 재장전 가능 상태인지 검사 (행동 차단 상태 + 무기 탄약 상태) */
	bool CanReload() const;

	/** 무기의 공격 몽타주가 아직 본 구간을 재생 중인지 여부 (블렌드 아웃에 들어가면 false → 다음 공격 허용) */
	bool IsAttackMontagePlaying() const;

	/** 머리 부품의 드랍률 배율 (빈 슬롯·파손 1, 손상이면 감소). 드랍 시스템이 읽는다 */
	float GetDropRateScale() const;

	/** 받는 피해 배율 (실드 능력이 조정). 0이면 피해·피격 반응 모두 없음 */
	FORCEINLINE void SetIncomingDamageScale(float InScale) { IncomingDamageScale = FMath::Clamp(InScale, 0.0f, 1.0f); }

	/** 코어 능력이 발동됐음을 알린다 (능력 → 캐릭터 → HUD) */
	void NotifyCoreAbilityActivated(const FText& InMessage);

	/** 코어 능력의 지속·쿨다운 상태가 바뀌었음을 알린다 (능력 → 캐릭터 → HUD) */
	void NotifyCoreAbilityStateChanged(bool bInActive, float InCooldownEndTime, float InCooldownDuration);

	/**
	 * 체력·부품·자원·탄약을 PlayerStatSubsystem에 저장한다.
	 * OpenLevel은 플레이어 폰을 EndPlay(LevelTransition)보다 먼저 파괴하므로 EndPlay에 기댈 수 없다.
	 * 스테이지 이동을 요청하는 UURGGameInstance가 이동 직전에 호출한다.
	 */
	void SaveStatSnapshot() const;

	/** 지금 발동 중인 시너지 태그 (HUD 초기 표시용) */
	FORCEINLINE const FGameplayTagContainer& GetActiveSynergies() const { return ActiveSynergies; }

	/** 강화 시너지 수치 (HUD 설명 문구용) */
	FORCEINLINE float GetReinforceMaxHealthBonus() const { return ReinforceMaxHealthBonus; }
	FORCEINLINE float GetReinforcePartDamageScale() const { return ReinforcePartDamageScale; }

	/** 코어 능력 발동 시 호출 */
	FDelegateOnCoreAbilityActivated OnCoreAbilityActivated;

	/** 코어 능력 발동·종료·교체 시 호출 (HUD 쿨다운 표시용) */
	FDelegateOnCoreAbilityStateChanged OnCoreAbilityStateChanged;

	/** 대시 사용·대시 불가 전환 시 호출 (HUD 다리 슬롯 쿨다운 표시용) */
	FDelegateOnDashCooldownChanged OnDashCooldownChanged;

	/** 공격·무기 교체 시 호출 (HUD 팔 슬롯 쿨다운 표시용) */
	FDelegateOnAttackCooldownChanged OnAttackCooldownChanged;

	/** 시너지가 켜지거나 꺼질 때 호출 */
	FDelegateOnSynergyChanged OnSynergyChanged;

	/** 지금 바라보는 상호작용 대상 (E를 누르면 상호작용할 액터). 없으면 nullptr */
	FORCEINLINE AActor* GetInteractFocus() const { return InteractFocus.Get(); }

	/** 바라보는 상호작용 대상이 바뀔 때 호출 */
	FDelegateOnInteractFocusChanged OnInteractFocusChanged;

protected:
	/** 새 행동(대시/공격/재장전)을 시작할 수 없는 상태인지 검사 (사망/대시·일반행동·피격 중) */
	bool IsActionBlocked() const;

	/** 사망 처리: 상태 잠금 후 베이스 처리 */
	virtual void HandleDeath() override;

	/** 저장된 스냅샷이 있으면 체력·부품·자원·탄약을 되돌린다 (없으면 기본 구성 그대로) */
	void RestoreStatSnapshot();

	/** 공통 공격 처리: 상태·공격 간격 검사 후 무기 공격 + 무기의 공격 몽타주. 무기가 공격 불가(탄약 없음)면 재장전 시도 */
	void TryAttack(const FGameplayTag& InAttackType);

	/** 현재 무기의 공격 쿨다운을 마지막 공격 시각 기준으로 알린다. 표시 기준 미만이거나 아직 공격 전이면 표시를 지운다 */
	void BroadcastAttackCooldown() const;

	/** 재장전 시작: 일반행동 상태로 전환하고 무기의 재장전 몽타주(없으면 ReloadDuration)만큼 대기 후 FinishReload */
	void TryReload();

	/** 재장전 대기 종료 (타이머 콜백): 무기 탄약을 채우고 일반행동 상태를 해제한다 */
	void FinishReload();

	/** 진행 중인 재장전 취소: 타이머를 지우고 일반행동 상태를 해제한다 (사망·무기 교체 시) */
	void CancelReload();

	/** 재장전으로 들어간 일반행동 상태 해제 (다른 상태로 덮였으면 유지) */
	void ClearReloadState();

	/** 부품 슬롯 변경 처리: 슬롯별 Apply*Part로 분기한 뒤 태그 시너지를 다시 계산 */
	void HandleSlotChanged(EURGPartSlot InSlot, const FURGPartSlotState& InState);

	/** 팔 슬롯 반영: 무기 클래스가 바뀌면 교체, 손상 단계를 공격력 배율로. 빈 슬롯·파손이면 기본 근접 무기 */
	void ApplyArmPart();

	/** 다리 슬롯 반영: 이동 속도·점프 횟수를 무브먼트에 쓰고 대시 가능 여부·배율을 보관. 빈 슬롯·파손이면 기본값(대시 불가) */
	void ApplyLegPart();

	/** 코어 슬롯 반영: 능력 클래스가 바뀌면 인스턴스 교체, 손상 단계를 쿨다운 배율로. 빈 슬롯·파손이면 능력 제거 */
	void ApplyCorePart();

	/** 머리 슬롯 반영: 치명타 확률(손상이면 감소)·배율을 CombatComponent에. 빈 슬롯·파손이면 치명타 0 */
	void ApplyHeadPart();

	/** 태그 시너지 반영: 강화는 최대 체력·부품 손상 배율, 열·전기는 CombatComponent의 적중 효과로 */
	void ApplySynergies();

	/** 시너지 발동 여부가 기록과 달라졌으면 기록을 고치고 OnSynergyChanged로 알린다 */
	void UpdateSynergyState(const FGameplayTag& InTag, bool bInActive);

	/** 카메라 시점 InteractTrace로 지금 상호작용할 수 있는 대상을 찾는다 (E 입력과 포커스 갱신이 같이 쓴다). 없으면 nullptr. CanInteract에 자신을 넘기므로 non-const */
	AActor* FindInteractTarget();

	/** 포커스 타이머 콜백: 바라보는 대상을 다시 찾고 바뀌었으면 알린다 */
	void UpdateInteractFocus();

	/** 포커스 대상 기록 + 바뀌었을 때만 OnInteractFocusChanged */
	void SetInteractFocus(AActor* InActor);

	/** 피격 결과의 뼈(없으면 충돌점에서 가장 가까운 바디 뼈)에서 부모로 올라가며 HitBoneSlotMap에 걸리는 부품 슬롯을 찾는다. 못 찾으면 false */
	bool ResolvePartSlotFromHit(const FHitResult& InHit, EURGPartSlot& OutSlot) const;

	/** 피격 처리 (AttributeComponent::OnDamaged): 재장전 취소, 신음, Hit 상태 진입, 방향별 상체 리액션 몽타주 재생 */
	void HandleDamaged(float InDamage, AActor* InCauser);

	/** 피해량에 맞는 신음 재생. 일반 신음은 PainSoundInterval 안에 다시 울리지 않는다 */
	void PlayPainSound(float InDamage);

	/** 피격 몽타주 종료: Hit 상태 해제 (다른 몽타주에 끊긴 경우는 그쪽이 정리) */
	void HandleHitReactEnded(class UAnimMontage* InMontage, bool bInterrupted);

/** 컴포넌트 */
protected:
	/** 플레이어 입력(매핑 컨텍스트/액션 바인딩) 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UURGPlayerInputComponent> PlayerInputComponent;

	/** 조준 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UURGAimComponent> AimComponent;

	/** 전투(주 무기) 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UURGCombatComponent> CombatComponent;

	/** 보유 자원 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UURGResourceComponent> ResourceComponent;

	/** 부품 슬롯 상태 컴포넌트 (팔 슬롯 → 무기 교체·공격력 배율, 다리 슬롯 → 대시 가능 여부) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UURGPartComponent> PartComponent;

	/** 적 탐지 레이더 (머리 부품 bRevealEnemies가 켠다) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UURGRadarComponent> RadarComponent;

/** 카메라 관련 컴포넌트 */
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UCameraComponent> Camera;

/** 이동 관련 변수 */
protected:
	/** 기본 이동 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Movement")
	float MoveSpeed = 600.0f;

	/** 점프 초기 수직 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Movement")
	float JumpVelocity = 600.0f;

/** 대시 관련 변수. 다리 부품(FURGLegStats)이 배율을 곱한다 */
protected:
	/** 대시 속도 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Dash")
	float DashSpeed = 1800.0f;

	/** 대시 지속 시간 (초). 이 동안 이동 입력이 잠긴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Dash")
	float DashDuration = 0.25f;

	/** 대시 쿨다운 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Dash")
	float DashCooldown = 1.0f;

/** 카메라 관련 변수 */
protected:
	/** 평상시 카메라 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Camera")
	float NormalArmLength = 300.0f;

/** 전투 관련 변수 (무기는 팔 부품이 공급한다 — PartComponent의 DefaultParts / EquipPart) */
protected:
	/** 연사 최소 간격 (초). 무기에 공격 몽타주가 있으면 몽타주 종료가 우선한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Combat")
	float RapidFireInterval = 0.12f;

	/** 무기 공격 쿨다운이 이 값(초) 이상일 때만 HUD에 표시한다 (짧은 간격까지 띄우면 슬롯이 계속 깜빡인다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Combat", meta = (ClampMin = "0"))
	float AttackCooldownDisplayMin = 0.5f;

	/** 재장전 소요 시간 (초). 무기에 재장전 몽타주가 없을 때 사용. 재장전 중에도 이동은 가능하다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Combat")
	float ReloadDuration = 1.5f;

	/** 팔 슬롯을 쓸 수 없을 때(빈 슬롯·파손) 드는 기본 근접 무기 */
	UPROPERTY(EditDefaultsOnly, Category = "Player|Combat")
	TSubclassOf<class AURGWeapon> DefaultArmWeaponClass;

/** 태그 시너지 관련 변수 (열·전기 수치는 CombatComponent) */
protected:
	/** 강화 시너지: 최대 체력 증가량 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Synergy", meta = (ClampMin = "0"))
	float ReinforceMaxHealthBonus = 30.0f;

	/** 강화 시너지: 부품 내구도 차감 배율 (작을수록 덜 손상) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Synergy", meta = (ClampMin = "0", ClampMax = "1"))
	float ReinforcePartDamageScale = 0.7f;

/** 상호작용 관련 변수 */
protected:
	/** 상호작용 탐색 거리 (캐릭터 기준, cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Interaction")
	float InteractRange = 300.0f;

	/** 바라보는 상호작용 대상을 다시 찾는 간격 (초). 매 프레임 대신 이 간격으로만 트레이스한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Interaction", meta = (ClampMin = "0.02"))
	float InteractFocusInterval = 0.1f;

protected:
	/** 피격 뼈 → 부품 슬롯. 부위 루트 뼈만 등록하면 하위 뼈는 부모 탐색으로 흡수된다. 어느 부위에도 안 걸리면 부품 손상 없음 */
	UPROPERTY(EditDefaultsOnly, Category = "Player|PartDamage")
	TMap<FName, EURGPartSlot> HitBoneSlotMap;

	/** 피격 방향별 상체 리액션 몽타주 (UpperBody 슬롯). 없는 방향은 리액션 생략 */
	UPROPERTY(EditDefaultsOnly, Category = "Player|Animation")
	TMap<EURGHitDirection, TObjectPtr<class UAnimMontage>> HitReactMontages;

	/** 사망 시 마지막 가해자 반대 방향으로 래그돌에 주는 속도 변화량 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Death")
	float DeathImpulse = 600.0f;

	/** 사망 밀침 방향에 섞는 위쪽 성분 비율 (0 = 수평, 1 = 45°) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Death", meta = (ClampMin = "0", ClampMax = "1"))
	float DeathImpulseUpRatio = 0.3f;

/** 음성 */
protected:
	/** 일반 피격 신음 */
	UPROPERTY(EditDefaultsOnly, Category = "Player|Sound")
	TObjectPtr<class USoundBase> PainSound;

	/** 큰 피격 신음 (한 번에 PainHeavyDamageRatio 이상 깎였을 때) */
	UPROPERTY(EditDefaultsOnly, Category = "Player|Sound")
	TObjectPtr<class USoundBase> PainHeavySound;

	/** 사망 비명 */
	UPROPERTY(EditDefaultsOnly, Category = "Player|Sound")
	TObjectPtr<class USoundBase> DeathSound;

	/** 큰 신음으로 바꾸는 피해 비율 (최대 체력 대비) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Sound", meta = (ClampMin = "0", ClampMax = "1"))
	float PainHeavyDamageRatio = 0.15f;

	/** 일반 신음 최소 간격 (초). 여러 적에게 동시에 맞아도 신음이 겹치지 않게 한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Sound", meta = (ClampMin = "0"))
	float PainSoundInterval = 0.4f;

#pragma region InputSystem

public:
	/** Movement */
	void Input_Move(const FInputActionValue& InputValue);
	void Input_Look(const FInputActionValue& InputValue);

	void Input_Jump(const FInputActionValue& InputValue);
	void Input_StopJump(const FInputActionValue& InputValue);
	void Input_Dash();

	/** Combat */
	void Input_SingleAttack();
	void Input_RapidAttack();

	void Input_Reload();
	void Input_Aim();
	void Input_StopAim();

	/** Interaction / Ability / System */
	void Input_Interact();
	void Input_CoreAbility();
	void Input_Pause();

#pragma endregion

private:
	/** 마지막 대시 시작 시각 (쿨다운 계산용) */
	float LastDashTime = -1.0f;

	/** 코어 부품이 제공하는 액티브 능력 인스턴스 (ApplyCorePart에서 생성·교체). 없으면 Q 무시 */
	UPROPERTY()
	TObjectPtr<class UURGCoreAbility> CoreAbility;

	/** 받는 피해 배율 (실드 능력용) */
	float IncomingDamageScale = 1.0f;

	/** 현재 다리 부품이 대시를 제공하는지 (ApplyLegPart에서 갱신). 대시는 기본 능력이 아니다 */
	bool bDashEnabled = false;

	/** 다리 부품·손상 단계가 정한 대시 속도 배율 (ApplyLegPart에서 갱신) */
	float DashSpeedScale = 1.0f;

	/** 다리 부품이 정한 대시 쿨다운 배율 (ApplyLegPart에서 갱신) */
	float DashCooldownScale = 1.0f;

	/** 강화 시너지의 최대 체력 보너스가 들어가 있는지 (켜고 끌 때만 더하고 빼기 위해 기록) */
	bool bReinforceApplied = false;

	/** 발동 중인 시너지 태그 (바뀐 태그만 OnSynergyChanged로 알리기 위해 기록) */
	FGameplayTagContainer ActiveSynergies;

	/** 마지막 발사 시각 (연사 간격 계산용) */
	float LastFireTime = -1.0f;

	/** 마지막 신음 재생 시각 (PainSoundInterval 계산용) */
	float LastPainSoundTime = -1.0f;

	/** 재장전 대기 타이머 핸들 */
	FTimerHandle ReloadTimer;

	/** 바라보는 상호작용 대상 갱신 타이머 */
	FTimerHandle InteractFocusTimer;

	/** 지금 바라보는 상호작용 대상 (픽업이 주워져 사라질 수 있으므로 약참조) */
	TWeakObjectPtr<AActor> InteractFocus;

};


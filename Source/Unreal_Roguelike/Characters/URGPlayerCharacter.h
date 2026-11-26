#pragma once

#include "CoreMinimal.h"
#include "Characters/URGCharacter.h"
#include "GameplayTagContainer.h"
#include "URGDefine.h"
#include "URGPlayerCharacter.generated.h"

struct FInputActionValue;
struct FURGPartSlotState;

/**
 * 플레이어 캐릭터 (회수 유닛).
 * 3인칭 카메라, 이동/점프를 담당하고 입력마다 행동 가능 여부와 상태 태그를 정한다.
 * 입력 바인딩은 PlayerInputComponent, 조준 카메라 연출은 AimComponent, 대시 쿨다운·발사는 DashComponent,
 * 상호작용 대상 탐색은 InteractComponent, 무기 보유·공격 간격·재장전 진행은 CombatComponent,
 * 보유 자원은 ResourceComponent, 부품 슬롯 상태는 PartComponent, Q 능력 인스턴스는 CoreAbilityComponent에 위임한다.
 * 부품 슬롯 변경(OnSlotChanged)을 받아 팔 부품 → 무기 교체·공격력 배율, 다리 부품 → 이동·대시 배율·점프 횟수,
 * 코어 부품 → CoreAbilityComponent의 능력 클래스·쿨다운 배율, 머리 부품 → 치명타(CombatComponent)·드랍률로 연결한다.
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
	FORCEINLINE class UURGCoreAbilityComponent* GetCoreAbilityComponent() const { return CoreAbilityComponent; }
	FORCEINLINE class UURGRadarComponent* GetRadarComponent() const { return RadarComponent; }
	FORCEINLINE class UURGDashComponent* GetDashComponent() const { return DashComponent; }
	FORCEINLINE class UURGInteractComponent* GetInteractComponent() const { return InteractComponent; }

	/** 현재 장착 중인 주 무기 (없으면 nullptr) */
	class AURGWeapon* GetMainWeapon() const;

	/** 이동·점프 입력을 받을 수 있는 상태인지 검사 (사망 + 이동 입력 잠금) */
	bool CanMove() const;

	/** 공격 가능 상태인지 검사 (행동 차단 상태 + 전투 활성화 + 무기 보유). 팔을 못 쓰면 기본 근접 무기를 든다 */
	bool CanAttack() const;

	/** 재장전 가능 상태인지 검사 (행동 차단 상태 + 무기 탄약 상태) */
	bool CanReload() const;

	/** 머리 부품의 드랍률 배율 (빈 슬롯·파손 1, 손상이면 감소). 드랍 시스템이 읽는다 */
	float GetDropRateScale() const;

	/**
	 * 체력·부품·자원·탄약을 PlayerStatSubsystem에 저장한다.
	 * OpenLevel은 플레이어 폰을 EndPlay(LevelTransition)보다 먼저 파괴하므로 EndPlay에 기댈 수 없다.
	 * 스테이지 이동을 요청하는 UURGGameInstance가 이동 직전에 호출한다.
	 */
	void SaveStatSnapshot() const;

	/** 강화 시너지 수치 (HUD 설명 문구용) */
	FORCEINLINE float GetReinforceMaxHealthBonus() const { return ReinforceMaxHealthBonus; }
	FORCEINLINE float GetReinforcePartDamageScale() const { return ReinforcePartDamageScale; }

protected:
	/** 새 행동(대시/공격/재장전)을 시작할 수 없는 상태인지 검사 (사망/대시·일반행동·피격 중) */
	bool IsActionBlocked() const;

	/** 사망 처리: 상태 잠금 후 베이스 처리 */
	virtual void HandleDeath() override;

	/** 저장된 스냅샷이 있으면 체력·부품·자원·탄약을 되돌린다 (없으면 기본 구성 그대로) */
	void RestoreStatSnapshot();

	/** 공통 공격 처리: 상태 검사 후 CombatComponent에 공격을 맡긴다. 탄약이 없으면 공격 대신 재장전 시도 */
	void TryAttack();

	/** 재장전 시작: 일반행동 상태로 전환하고 CombatComponent에 재장전을 맡긴다. 끝나면 HandleReloadEnded */
	void TryReload();

	/** 재장전 종료·취소 (CombatComponent::OnReloadEnded): 재장전으로 들어간 일반행동 상태 해제 (다른 상태로 덮였으면 유지) */
	void HandleReloadEnded(bool bInCompleted);

	/** 대시 종료 (DashComponent::OnDashEnded): 이동 입력을 풀고, 아직 대시 상태면 상태를 지운다 */
	void HandleDashEnded();

	/** 부품 슬롯 변경 처리 (PartComponent::OnSlotChanged): 슬롯별 Apply*Part로 분기 */
	void HandleSlotChanged(EURGPartSlot InSlot, const FURGPartSlotState& InState);

	/** 팔 슬롯 반영: 무기 클래스가 바뀌면 교체, 손상 단계를 공격력 배율로. 빈 슬롯·파손이면 기본 근접 무기 */
	void ApplyArmPart();

	/** 다리 슬롯 반영: 이동 속도·점프 횟수를 무브먼트에 쓰고 대시 가능 여부·배율을 DashComponent에. 빈 슬롯·파손이면 기본값(대시 불가) */
	void ApplyLegPart();

	/** 코어 슬롯 반영: 능력 클래스와 손상 단계의 쿨다운 배율을 CoreAbilityComponent에. 빈 슬롯·파손이면 능력 제거 */
	void ApplyCorePart();

	/** 머리 슬롯 반영: 치명타 확률(손상이면 감소)·배율을 CombatComponent에. 빈 슬롯·파손이면 치명타 0 */
	void ApplyHeadPart();

	/** 시너지 발동·해제 (PartComponent::OnSynergyChanged): 강화는 최대 체력·부품 손상 배율, 열·전기는 CombatComponent의 적중 효과로 */
	void HandleSynergyChanged(const FGameplayTag& InTag, bool bInActive);

	/** 피격 처리 (AttributeComponent::OnDamaged): 재장전 취소 후 HitReactComponent 연출, 리액션이 재생되면 Hit 상태 진입 */
	void HandleDamaged(float InDamage, AActor* InCauser);

	/** 피격 리액션 종료 (HitReactComponent::OnHitReactEnded): Hit 상태 해제 (다른 몽타주에 끊긴 경우는 그쪽이 정리) */
	void HandleHitReactEnded(bool bInterrupted);

/** 컴포넌트 */
protected:
	/** 플레이어 입력(매핑 컨텍스트/액션 바인딩) 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Components")
	TObjectPtr<class UURGPlayerInputComponent> PlayerInputComponent;

	/** 조준 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Components")
	TObjectPtr<class UURGAimComponent> AimComponent;

	/** 전투(주 무기) 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Components")
	TObjectPtr<class UURGCombatComponent> CombatComponent;

	/** 보유 자원 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Components")
	TObjectPtr<class UURGResourceComponent> ResourceComponent;

	/** 부품 슬롯 상태 컴포넌트 (팔 슬롯 → 무기 교체·공격력 배율, 다리 슬롯 → 대시 가능 여부) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Components")
	TObjectPtr<class UURGPartComponent> PartComponent;

	/** 코어 부품이 주는 Q 능력 인스턴스 보관·교체·발동 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Components")
	TObjectPtr<class UURGCoreAbilityComponent> CoreAbilityComponent;

	/** 적 탐지 레이더 (머리 부품 bRevealEnemies가 켠다) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Components")
	TObjectPtr<class UURGRadarComponent> RadarComponent;

	/** 피격·사망 연출 (리액션 몽타주, 신음, 비명, 래그돌 밀침) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Components")
	TObjectPtr<class UURGHitReactComponent> HitReactComponent;

	/** 대시 쿨다운·방향·발사 (다리 부품이 허용 여부와 배율을 정한다) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Components")
	TObjectPtr<class UURGDashComponent> DashComponent;

	/** 바라보는 상호작용 대상 탐색과 상호작용 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Components")
	TObjectPtr<class UURGInteractComponent> InteractComponent;

/** 카메라 관련 컴포넌트 */
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Camera")
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Camera")
	TObjectPtr<class UCameraComponent> Camera;

/** 이동 관련 변수 */
protected:
	/** 기본 이동 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Movement")
	float MoveSpeed = 600.0f;

	/** 점프 초기 수직 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Movement")
	float JumpVelocity = 600.0f;

/** 전투 관련 변수 (무기는 팔 부품이 공급한다 — PartComponent의 DefaultParts / EquipPart. 공격 간격·재장전 값은 CombatComponent) */
protected:
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

#pragma region InputSystem

public:
	/** Movement */
	void Input_Move(const FInputActionValue& InValue);
	void Input_Look(const FInputActionValue& InValue);

	void Input_Jump(const FInputActionValue& InValue);
	void Input_StopJump(const FInputActionValue& InValue);
	void Input_Dash();

	/** Combat */
	void Input_Attack();

	void Input_Reload();
	void Input_Aim();
	void Input_StopAim();

	/** Interaction / Ability / System */
	void Input_Interact();
	void Input_CoreAbility();
	void Input_Pause();

#pragma endregion

};


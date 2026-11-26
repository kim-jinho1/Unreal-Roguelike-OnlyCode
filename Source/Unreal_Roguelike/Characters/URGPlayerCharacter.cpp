#include "Characters/URGPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Component/URGAimComponent.h"
#include "Component/URGCombatComponent.h"
#include "Component/URGCoreAbilityComponent.h"
#include "Component/URGDashComponent.h"
#include "Component/URGHitReactComponent.h"
#include "Component/URGInteractComponent.h"
#include "Component/URGPartComponent.h"
#include "Component/URGPlayerInputComponent.h"
#include "Component/URGRadarComponent.h"
#include "Component/URGResourceComponent.h"
#include "Component/URGStateComponent.h"
#include "Component/URGAttributeComponent.h"
#include "Core/Subsystem/PlayerStatSubsystem.h"
#include "Core/URGPlayerController.h"
#include "Engine/DamageEvents.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Equipment/URGPartDataAsset.h"
#include "Equipment/URGRangedWeapon.h"
#include "Equipment/URGWeapon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "InputActionValue.h"
#include "URGDefine.h"
#include "URGGameplayTags.h"

namespace
{
	/** 카메라 스프링암 기본 내려다보는 각도 (도) */
	constexpr float CameraPitch = -30.0f;

	/** 평상시 카메라 거리 (cm). 조준 해제 시 AimComponent가 BeginPlay에서 읽어 둔 스프링암 길이로 돌아온다 */
	constexpr float NormalArmLength = 300.0f;

	/** 이동 입력을 놓았을 때 걷기 감속도 (cm/s²) */
	constexpr float WalkBrakingDeceleration = 2000.0f;

	/** 이동 방향으로 몸을 돌리는 회전 속도 (도/초) */
	constexpr float TurnRateYaw = 720.0f;
}

AURGPlayerCharacter::AURGPlayerCharacter()
{
	// Camera
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = NormalArmLength;
	SpringArm->SetRelativeRotation(FRotator(CameraPitch, 0.0f, 0.0f));
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bEnableCameraLag = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	// Character Movement (회전 방식은 베이스: 이동 방향 회전, 조준 중에만 AimComponent가 카메라 방향으로 전환)
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = MoveSpeed;
		MovementComponent->JumpZVelocity = JumpVelocity;
		MovementComponent->BrakingDecelerationWalking = WalkBrakingDeceleration;
		MovementComponent->RotationRate = FRotator(0.0f, TurnRateYaw, 0.0f);
	}

	// Input / Aim / Dash / Interact / Combat / Resource / Part / CoreAbility
	PlayerInputComponent = CreateDefaultSubobject<UURGPlayerInputComponent>(TEXT("PlayerInput"));
	AimComponent = CreateDefaultSubobject<UURGAimComponent>(TEXT("Aim"));
	DashComponent = CreateDefaultSubobject<UURGDashComponent>(TEXT("Dash"));
	InteractComponent = CreateDefaultSubobject<UURGInteractComponent>(TEXT("Interact"));
	CombatComponent = CreateDefaultSubobject<UURGCombatComponent>(TEXT("Combat"));
	ResourceComponent = CreateDefaultSubobject<UURGResourceComponent>(TEXT("Resource"));
	PartComponent = CreateDefaultSubobject<UURGPartComponent>(TEXT("Part"));
	CoreAbilityComponent = CreateDefaultSubobject<UURGCoreAbilityComponent>(TEXT("CoreAbility"));
	RadarComponent = CreateDefaultSubobject<UURGRadarComponent>(TEXT("Radar"));
	HitReactComponent = CreateDefaultSubobject<UURGHitReactComponent>(TEXT("HitReact"));
}

void AURGPlayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// PartComponent가 BeginPlay에서 기본 부품을 장착하므로 그 전에 구독해 둔다.
	if (PartComponent)
	{
		PartComponent->OnSlotChanged.AddUObject(this, &AURGPlayerCharacter::HandleSlotChanged);
		PartComponent->OnSynergyChanged.AddUObject(this, &AURGPlayerCharacter::HandleSynergyChanged);
	}

	if (AttributeComponent)
	{
		AttributeComponent->OnDamaged.AddUObject(this, &AURGPlayerCharacter::HandleDamaged);
	}

	if (HitReactComponent)
	{
		HitReactComponent->OnHitReactEnded.AddUObject(this, &AURGPlayerCharacter::HandleHitReactEnded);
	}

	if (DashComponent)
	{
		DashComponent->OnDashEnded.AddUObject(this, &AURGPlayerCharacter::HandleDashEnded);
	}

	if (CombatComponent)
	{
		CombatComponent->OnReloadEnded.AddUObject(this, &AURGPlayerCharacter::HandleReloadEnded);
	}
}

void AURGPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// BP에서 값을 바꿨을 수 있으므로 시작 시 한 번 더 반영한다.
	// 이동 속도는 Super::BeginPlay 안에서 PartComponent가 기본 다리를 장착하며 이미 배율을 곱했을 수 있으니 직접 쓰지 않고 다리 반영을 다시 태운다.
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->JumpZVelocity = JumpVelocity;
	}

	ApplyLegPart();

	// 이전 스테이지에서 저장해 둔 상태로 덮어쓴다. 저장이 없으면(첫 맵) 아무 일도 하지 않는다.
	RestoreStatSnapshot();
}

void AURGPlayerCharacter::SaveStatSnapshot() const
{
	// 사망으로 끝난 판은 이어 갈 이유가 없다.
	if (IsDead())
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UPlayerStatSubsystem* StatSubsystem = GameInstance ? GameInstance->GetSubsystem<UPlayerStatSubsystem>() : nullptr;
	if (StatSubsystem == nullptr)
	{
		return;
	}

	FURGPlayerSnapshot Snapshot;

	if (AttributeComponent)
	{
		Snapshot.Health = AttributeComponent->GetCurrentHealth();
		Snapshot.MaxHealth = AttributeComponent->GetMaxHealth();
	}

	if (ResourceComponent)
	{
		Snapshot.Resource = ResourceComponent->GetResource();
	}

	if (PartComponent)
	{
		Snapshot.Slots = PartComponent->GetSlots();
	}

	// 근접 무기거나 무기가 없으면 탄약을 저장하지 않는다 (-1 그대로).
	if (const AURGRangedWeapon* RangedWeapon = Cast<AURGRangedWeapon>(GetMainWeapon()))
	{
		Snapshot.Ammo = RangedWeapon->GetCurrentAmmo();
	}

	StatSubsystem->SaveSnapshot(Snapshot);
}

void AURGPlayerCharacter::RestoreStatSnapshot()
{
	UGameInstance* GameInstance = GetGameInstance();
	UPlayerStatSubsystem* StatSubsystem = GameInstance ? GameInstance->GetSubsystem<UPlayerStatSubsystem>() : nullptr;
	if (StatSubsystem == nullptr || !StatSubsystem->HasSnapshot())
	{
		return;
	}

	const FURGPlayerSnapshot& Snapshot = StatSubsystem->GetSnapshot();

	// 부품을 가장 먼저 되돌린다. 슬롯 변경이 무기 교체·이동 배율·치명타까지 다시 태우므로
	// 체력과 탄약은 그 뒤에 써야 부품 반영 결과에 덮이지 않는다.
	if (PartComponent)
	{
		PartComponent->RestoreSlots(Snapshot.Slots);
	}

	if (AttributeComponent)
	{
		AttributeComponent->RestoreHealth(Snapshot.Health, Snapshot.MaxHealth);
	}

	if (ResourceComponent)
	{
		ResourceComponent->SetResource(Snapshot.Resource);
	}

	if (Snapshot.Ammo >= 0)
	{
		if (AURGRangedWeapon* RangedWeapon = Cast<AURGRangedWeapon>(GetMainWeapon()))
		{
			RangedWeapon->RestoreAmmo(Snapshot.Ammo);
		}
	}
}

void AURGPlayerCharacter::HandleSlotChanged(EURGPartSlot InSlot, const FURGPartSlotState& InState)
{
	switch (InSlot)
	{
	case EURGPartSlot::Arm:
		ApplyArmPart();
		break;
	case EURGPartSlot::Leg:
		ApplyLegPart();
		break;
	case EURGPartSlot::Core:
		ApplyCorePart();
		break;
	case EURGPartSlot::Head:
		ApplyHeadPart();
		break;
	default:
		break;
	}
}

void AURGPlayerCharacter::ApplyArmPart()
{
	if (CombatComponent == nullptr || PartComponent == nullptr)
	{
		return;
	}

	// 빈 슬롯·파손이면 기본 근접 무기로 싸운다. 파손된 부품은 슬롯에 남아 있으므로 수리하면 원래 무기로 돌아온다
	const bool bUsable = PartComponent->IsSlotUsable(EURGPartSlot::Arm);
	const TSubclassOf<AURGWeapon> WeaponClass = bUsable ? PartComponent->GetSlotState(EURGPartSlot::Arm).Part->GetWeaponClass() : DefaultArmWeaponClass;

	// 무기 클래스가 현재 무기와 다를 때만 교체한다 (손상 단계만 바뀐 경우 탄약을 유지)
	const AURGWeapon* CurrentWeapon = CombatComponent->GetMainWeapon();
	const UClass* CurrentWeaponClass = CurrentWeapon != nullptr ? CurrentWeapon->GetClass() : nullptr;

	// 진행 중인 재장전 취소와 쿨다운 다시 알리기는 EquipWeapon이 한다
	if (CurrentWeaponClass != WeaponClass.Get())
	{
		CombatComponent->EquipWeapon(WeaponClass);
	}

	// 기본 근접 무기는 부품이 아니므로 손상 배율을 받지 않는다 (파손 배율 0을 곱하면 데미지가 사라진다)
	CombatComponent->SetAttackDamageScale(bUsable ? PartComponent->GetConditionScale(EURGPartSlot::Arm) : 1.0f);
}

void AURGPlayerCharacter::ApplyLegPart()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (MovementComponent == nullptr)
	{
		return;
	}

	// 빈 슬롯·파손은 부품 배율 없이 기본값으로 걷고 대시할 수 없다
	const bool bUsable = PartComponent != nullptr && PartComponent->IsSlotUsable(EURGPartSlot::Leg);
	if (!bUsable)
	{
		MovementComponent->MaxWalkSpeed = MoveSpeed;
		JumpMaxCount = 1;

		check(DashComponent);
		DashComponent->SetLegStats(false, 1.0f, 1.0f);
		return;
	}

	const FURGPartSlotState& State = PartComponent->GetSlotState(EURGPartSlot::Leg);
	const FURGLegStats& Stats = State.Part->GetLegStats();

	// 손상(Damaged)이면 속도 배율이 깎이고 다단 점프를 잃는다. 쿨다운은 손상과 무관
	const float ConditionScale = PartComponent->GetConditionScale(EURGPartSlot::Leg);
	const bool bDamaged = State.Condition == EURGPartCondition::Damaged;

	MovementComponent->MaxWalkSpeed = MoveSpeed * Stats.MoveSpeedScale * ConditionScale;
	JumpMaxCount = bDamaged ? 1 : Stats.JumpMaxCount;

	check(DashComponent);
	DashComponent->SetLegStats(Stats.bCanDash, Stats.DashSpeedScale * ConditionScale, Stats.DashCooldownScale);
}

void AURGPlayerCharacter::ApplyCorePart()
{
	if (PartComponent == nullptr)
	{
		return;
	}

	check(CoreAbilityComponent);

	// 빈 슬롯·파손이면 능력을 비운다. 이때는 배율을 쓰지 않으므로 파손 배율 0으로 나누지 않게 1을 넘긴다
	if (!PartComponent->IsSlotUsable(EURGPartSlot::Core))
	{
		CoreAbilityComponent->SetAbility(nullptr, 1.0f);
		return;
	}

	// 손상(Damaged)이면 쿨다운이 길어진다
	const UURGPartDataAsset* CorePart = PartComponent->GetSlotState(EURGPartSlot::Core).Part;
	CoreAbilityComponent->SetAbility(CorePart->GetCoreAbilityClass(), 1.0f / PartComponent->GetConditionScale(EURGPartSlot::Core));
}

void AURGPlayerCharacter::ApplyHeadPart()
{
	if (CombatComponent == nullptr || PartComponent == nullptr)
	{
		return;
	}

	if (!PartComponent->IsSlotUsable(EURGPartSlot::Head))
	{
		CombatComponent->SetCritStats(0.0f, 1.0f);
		if (RadarComponent != nullptr)
		{
			RadarComponent->SetRadarEnabled(false);
		}
		return;
	}

	// 손상(Damaged)이면 치명타 확률이 깎인다. 배율은 그대로
	const FURGHeadStats& Stats = PartComponent->GetSlotState(EURGPartSlot::Head).Part->GetHeadStats();
	CombatComponent->SetCritStats(Stats.CritChance * PartComponent->GetConditionScale(EURGPartSlot::Head), Stats.CritDamageScale);

	// 적 탐지는 파손 전까지 동작한다
	if (RadarComponent != nullptr)
	{
		RadarComponent->SetRadarEnabled(Stats.bRevealEnemies);
	}
}

void AURGPlayerCharacter::HandleSynergyChanged(const FGameplayTag& InTag, bool bInActive)
{
	// 강화: 알림은 켜지고 꺼지는 순간에만 오므로 최대 체력 보너스를 그대로 더하고 빼도 짝이 맞는다
	if (InTag == URGGameplayTags::Part_Tag_Reinforce)
	{
		if (AttributeComponent != nullptr)
		{
			AttributeComponent->AddMaxHealth(bInActive ? ReinforceMaxHealthBonus : -ReinforceMaxHealthBonus);
		}

		check(PartComponent);
		PartComponent->SetPartDamageScale(bInActive ? ReinforcePartDamageScale : 1.0f);
		return;
	}

	// 열·전기: 적중 효과는 무기가 데미지를 낼 때 CombatComponent가 붙인다
	if (InTag == URGGameplayTags::Part_Tag_Thermal || InTag == URGGameplayTags::Part_Tag_Electric)
	{
		check(PartComponent);
		check(CombatComponent);
		CombatComponent->SetSynergyEffects(PartComponent->IsSynergyActive(URGGameplayTags::Part_Tag_Thermal), PartComponent->IsSynergyActive(URGGameplayTags::Part_Tag_Electric));
	}
}

float AURGPlayerCharacter::GetDropRateScale() const
{
	if (PartComponent == nullptr || !PartComponent->IsSlotUsable(EURGPartSlot::Head))
	{
		return 1.0f;
	}

	// 손상이면 보너스분만 깎인다 (1.5배 → 1.35배). 기본 1 아래로는 안 떨어진다
	const float Bonus = PartComponent->GetSlotState(EURGPartSlot::Head).Part->GetHeadStats().DropRateScale - 1.0f;
	return 1.0f + Bonus * PartComponent->GetConditionScale(EURGPartSlot::Head);
}

void AURGPlayerCharacter::HandleDamaged(float InDamage, AActor* InCauser)
{	// 사망 처리는 HandleDeath가, 대시 중에는 데미지만 받고 리액션은 생략한다.
	if (IsDead() || StateComponent->IsCurrentStateEqualTo(URGGameplayTags::Character_State_Dashing))
	{
		return;
	}

	// 피격은 재장전을 끊는다 (재장전 중이 아니면 아무 일도 하지 않는다).
	check(CombatComponent);
	CombatComponent->CancelReload();

	check(HitReactComponent);

	// 컴포넌트끼리 직접 참조하지 않도록 최대 체력 대비 비율은 캐릭터가 계산해 넘긴다
	const float MaxHealth = AttributeComponent ? AttributeComponent->GetMaxHealth() : 0.0f;
	const float DamageRatio = MaxHealth > 0.0f ? InDamage / MaxHealth : 0.0f;

	// 몽타주가 없는 방향이면 리액션도 상태 진입도 생략한다 (풀어 줄 계기가 없으므로).
	if (HitReactComponent->PlayHitReact(DamageRatio, InCauser))
	{
		StateComponent->SetCurrentState(URGGameplayTags::Character_State_Hit);
	}
}

void AURGPlayerCharacter::HandleHitReactEnded(bool bInterrupted)
{
	// 다음 피격 몽타주나 사망에 끊긴 경우는 그쪽이 상태를 책임진다.
	if (bInterrupted)
	{
		return;
	}

	if (StateComponent->IsCurrentStateEqualTo(URGGameplayTags::Character_State_Hit))
	{
		StateComponent->ClearCurrentState();
	}
}

void AURGPlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// 기본 매핑 컨텍스트 등록은 입력 컴포넌트에 위임
	if (PlayerInputComponent)
	{
		PlayerInputComponent->AddMappingContext();
	}
}

void AURGPlayerCharacter::SetupPlayerInputComponent(UInputComponent* InPlayerInputComponent)
{
	Super::SetupPlayerInputComponent(InPlayerInputComponent);

	// 액션 바인딩은 입력 컴포넌트에 위임
	if (PlayerInputComponent)
	{
		PlayerInputComponent->BindInputActions(InPlayerInputComponent);
	}

}

float AURGPlayerCharacter::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 실드가 피해를 전부 막으면 엔진 파이프라인(OnTakeAnyDamage → 체력·피격 반응)에 아예 넣지 않는다
	check(CoreAbilityComponent);
	Damage *= CoreAbilityComponent->GetIncomingDamageScale();
	if (Damage <= 0.0f)
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage <= 0.0f || IsDead() || PartComponent == nullptr)
	{
		return ActualDamage;
	}

	// 점 피해(적 근접 공격·투사체 직격)만 맞은 뼈로 부품 칸을 찾아 부품도 깎는다.
	// 범위 피해(폭발·보스 내려찍기)와 일반 피해(보스 휩쓸기·화상)는 맞은 위치가 없어 체력만 깎인다.
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PointEvent = static_cast<const FPointDamageEvent&>(DamageEvent);

		EURGPartSlot Slot;
		if (PartComponent->ResolveSlotFromHit(PointEvent.HitInfo, GetMesh(), Slot))
		{
			PartComponent->ApplyPartDamage(Slot, ActualDamage);
		}
	}

	return ActualDamage;
}

AURGWeapon* AURGPlayerCharacter::GetMainWeapon() const
{
	return CombatComponent ? CombatComponent->GetMainWeapon() : nullptr;
}

bool AURGPlayerCharacter::IsActionBlocked() const
{
	if (IsDead())
	{
		return true;
	}

	check(StateComponent);

	// 대시/재장전(일반행동)/피격 중에는 새 행동을 시작할 수 없다
	FGameplayTagContainer BlockingTags;
	BlockingTags.AddTag(URGGameplayTags::Character_State_Dashing);
	BlockingTags.AddTag(URGGameplayTags::Character_State_GeneralAction);
	BlockingTags.AddTag(URGGameplayTags::Character_State_Hit);

	return StateComponent->IsCurrentStateEqualToAny(BlockingTags);
}

bool AURGPlayerCharacter::CanMove() const
{
	if (IsDead())
	{
		return false;
	}

	check(StateComponent);
	return StateComponent->IsMovementInputEnabled();
}

bool AURGPlayerCharacter::CanAttack() const
{
	if (IsActionBlocked())
	{
		return false;
	}

	// 팔을 못 써도 기본 근접 무기를 들고 있으므로, 무기 자체가 없을 때만 막는다
	if (GetMainWeapon() == nullptr)
	{
		return false;
	}

	return CombatComponent && CombatComponent->IsCombatEnabled();
}

bool AURGPlayerCharacter::CanReload() const
{
	if (IsActionBlocked())
	{
		return false;
	}

	check(CombatComponent);
	return CombatComponent->CanReload();
}

void AURGPlayerCharacter::HandleDeath()
{
	check(CombatComponent);
	CombatComponent->CancelReload();

	// 대시 중에 죽으면 대시 종료가 늦게 와서 이동 입력을 다시 풀지 못하게 취소한다
	check(DashComponent);
	DashComponent->CancelDash();

	// 조준 중에 죽으면 줌 FOV와 조준 회전이 남아 래그돌 몸통을 계속 돌리므로 평상시로 되돌린다
	if (AimComponent)
	{
		AimComponent->StopAim();
	}

	// 죽은 뒤에는 상호작용할 수 없으므로 비교 UI 등이 남지 않게 포커스를 비운다
	check(InteractComponent);
	InteractComponent->StopFocus();

	// 적 마커도 지운다
	if (RadarComponent)
	{
		RadarComponent->SetRadarEnabled(false);
	}

	// 시간 감속 같은 지속 효과가 사망 뒤에도 남지 않게 끊는다
	check(CoreAbilityComponent);
	CoreAbilityComponent->Cancel();

	// 공격·피격 몽타주를 끊는다.
	StopAnimMontage();

	Super::HandleDeath();

	// 비명·래그돌·밀침 연출은 베이스가 이동을 멈추고 캡슐 충돌을 끈 뒤에 한다.
	// 가해자는 AttributeComponent가 기록해 두므로 캐릭터가 꺼내 넘긴다 (컴포넌트끼리 직접 참조하지 않는다).
	check(HitReactComponent);
	const AActor* Causer = AttributeComponent ? AttributeComponent->GetLastDamageCauser() : nullptr;
	HitReactComponent->PlayDeath(Causer);
}

void AURGPlayerCharacter::TryAttack()
{
	if (!CanAttack())
	{
		return;
	}

	// 탄약이 없으면 공격 대신 자동 재장전. 공격 모션·간격 검사와 몽타주는 CombatComponent가 맡는다
	if (CombatComponent->NeedsReload())
	{
		TryReload();
		return;
	}

	CombatComponent->TryAttack();
}

void AURGPlayerCharacter::TryReload()
{
	if (!CanReload())
	{
		return;
	}

	// 재장전 동안 일반행동 상태(발사/대시 차단). 이동은 허용한다. CombatComponent가 OnReloadEnded로 알리면 HandleReloadEnded에서 푼다
	check(StateComponent);
	StateComponent->SetCurrentState(URGGameplayTags::Character_State_GeneralAction);

	CombatComponent->StartReload();
}

void AURGPlayerCharacter::HandleReloadEnded(bool bInCompleted)
{
	// 재장전 중 다른 상태로 덮였으면 그 상태의 주인이 지우도록 둔다
	check(StateComponent);
	if (StateComponent->IsCurrentStateEqualTo(URGGameplayTags::Character_State_GeneralAction))
	{
		StateComponent->ClearCurrentState();
	}
}

void AURGPlayerCharacter::Input_Move(const FInputActionValue& InValue)
{
	if (!CanMove())
	{
		return;
	}

	const FVector2D MovementVector = InValue.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);

		const FVector ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardVector, MovementVector.X);
		AddMovementInput(RightVector, MovementVector.Y);
	}
}

void AURGPlayerCharacter::Input_Look(const FInputActionValue& InValue)
{
	const FVector2D LookVector = InValue.Get<FVector2D>();

	AddControllerYawInput(LookVector.X);
	AddControllerPitchInput(LookVector.Y);
}

void AURGPlayerCharacter::Input_Jump(const FInputActionValue& InValue)
{
	if (!CanMove())
	{
		return;
	}

	if (InValue.Get<bool>())
	{
		Jump();
	}
}

void AURGPlayerCharacter::Input_StopJump(const FInputActionValue& InValue)
{
	if (!InValue.Get<bool>())
	{
		StopJumping();
	}
}

void AURGPlayerCharacter::Input_Dash()
{
	// 행동 가능 여부는 캐릭터가, 다리 부품 허용·쿨다운은 DashComponent가 본다
	check(DashComponent);
	if (IsActionBlocked() || !DashComponent->CanDash())
	{
		return;
	}

	// 대시 동안 이동 입력을 잠그고, DashComponent가 DashDuration 뒤 OnDashEnded로 알리면 HandleDashEnded에서 푼다
	check(StateComponent);
	StateComponent->SetCurrentState(URGGameplayTags::Character_State_Dashing);
	StateComponent->SetMovementInputEnabled(false);

	DashComponent->StartDash();
}

void AURGPlayerCharacter::HandleDashEnded()
{
	check(StateComponent);
	StateComponent->SetMovementInputEnabled(true);

	// 대시 도중 다른 상태로 바뀌었으면 그 상태의 주인이 지우도록 둔다
	if (StateComponent->IsCurrentStateEqualTo(URGGameplayTags::Character_State_Dashing))
	{
		StateComponent->ClearCurrentState();
	}
}

void AURGPlayerCharacter::Input_Attack()
{
	TryAttack();
}

void AURGPlayerCharacter::Input_Reload()
{
	TryReload();
}

void AURGPlayerCharacter::Input_Aim()
{
	if (IsDead() || AimComponent == nullptr)
	{
		return;
	}

	AimComponent->Aim();
}

void AURGPlayerCharacter::Input_StopAim()
{
	if (AimComponent == nullptr)
	{
		return;
	}

	AimComponent->StopAim();
}

void AURGPlayerCharacter::Input_Interact()
{
	if (IsDead())
	{
		return;
	}

	check(InteractComponent);
	InteractComponent->TryInteract();
}

void AURGPlayerCharacter::Input_CoreAbility()
{
	if (IsDead())
	{
		return;
	}

	// 코어 부품이 없거나 파손이면 ApplyCorePart가 능력을 비워 두므로 아무 일도 없다
	check(CoreAbilityComponent);
	CoreAbilityComponent->TryActivate();
}

void AURGPlayerCharacter::Input_Pause()
{
	// 일시정지 화면은 컨트롤러 델리게이트를 구독하는 UI 담당이 처리한다.
	if (AURGPlayerController* PlayerController = Cast<AURGPlayerController>(GetController()))
	{
		PlayerController->RequestPause();
	}
}



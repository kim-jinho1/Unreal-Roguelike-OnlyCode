#include "Characters/URGPlayerCharacter.h"
#include "Ability/URGCoreAbility.h"
#include "Camera/CameraComponent.h"
#include "Component/URGAimComponent.h"
#include "Component/URGCombatComponent.h"
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
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "InputActionValue.h"
#include "Interface/URGInteractable.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "URGDefine.h"
#include "URGGameplayTags.h"

AURGPlayerCharacter::AURGPlayerCharacter()
{
	// Camera
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = NormalArmLength;
	SpringArm->SetRelativeRotation(FRotator(-30, 0, 0));
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bEnableCameraLag = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	// Character Movement (회전 방식은 베이스: 이동 방향 회전, 조준 중에만 AimComponent가 카메라 방향으로 전환)
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = MoveSpeed;
		MovementComponent->JumpZVelocity = JumpVelocity;
		MovementComponent->BrakingDecelerationWalking = 2000.0f;
		MovementComponent->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	}

	// Input / Aim / Combat / Resource / Part
	PlayerInputComponent = CreateDefaultSubobject<UURGPlayerInputComponent>(TEXT("PlayerInput"));
	AimComponent = CreateDefaultSubobject<UURGAimComponent>(TEXT("Aim"));
	CombatComponent = CreateDefaultSubobject<UURGCombatComponent>(TEXT("Combat"));
	ResourceComponent = CreateDefaultSubobject<UURGResourceComponent>(TEXT("Resource"));
	PartComponent = CreateDefaultSubobject<UURGPartComponent>(TEXT("Part"));
	RadarComponent = CreateDefaultSubobject<UURGRadarComponent>(TEXT("Radar"));
}

void AURGPlayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// PartComponent가 BeginPlay에서 기본 부품을 장착하므로 그 전에 구독해 둔다.
	if (PartComponent)
	{
		PartComponent->OnSlotChanged.AddUObject(this, &AURGPlayerCharacter::HandleSlotChanged);
	}

	if (AttributeComponent)
	{
		AttributeComponent->OnDamaged.AddUObject(this, &AURGPlayerCharacter::HandleDamaged);
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

	// 바라보는 상호작용 대상(부품 비교 UI 등)은 매 프레임이 아니라 짧은 간격으로만 찾는다
	GetWorldTimerManager().SetTimer(InteractFocusTimer, this, &AURGPlayerCharacter::UpdateInteractFocus, InteractFocusInterval, true);
}

void AURGPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(InteractFocusTimer);

	Super::EndPlay(EndPlayReason);
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

void AURGPlayerCharacter::HandleSlotChanged(const EURGPartSlot InSlot, const FURGPartSlotState& InState)
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

	// 어느 슬롯이 바뀌든 태그 개수가 달라질 수 있다 (장착·교체·파손·수리 모두)
	ApplySynergies();
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
	const UClass* CurrentWeaponClass = CurrentWeapon ? CurrentWeapon->GetClass() : nullptr;

	if (CurrentWeaponClass != WeaponClass.Get())
	{
		// 이전 무기에 걸려 있던 재장전은 새 무기에 적용되면 안 된다
		CancelReload();
		CombatComponent->EquipWeapon(WeaponClass);

		// 쿨다운은 마지막 공격 시각 기준이므로 새 무기의 쿨다운 길이로 남은 시간을 다시 계산해 보여 준다
		BroadcastAttackCooldown();
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
	const bool bUsable = PartComponent && PartComponent->IsSlotUsable(EURGPartSlot::Leg);
	if (!bUsable)
	{
		MovementComponent->MaxWalkSpeed = MoveSpeed;
		JumpMaxCount = 1;
		bDashEnabled = false;
		DashSpeedScale = 1.0f;
		DashCooldownScale = 1.0f;

		// 쿨다운 도중 다리를 잃으면 남은 쿨다운 표시도 지운다
		OnDashCooldownChanged.Broadcast(0.0f, 0.0f);
		return;
	}

	const FURGPartSlotState& State = PartComponent->GetSlotState(EURGPartSlot::Leg);
	const FURGLegStats& Stats = State.Part->GetLegStats();

	// 손상(Damaged)이면 속도 배율이 깎이고 다단 점프를 잃는다. 쿨다운은 손상과 무관
	const float ConditionScale = PartComponent->GetConditionScale(EURGPartSlot::Leg);
	const bool bDamaged = State.Condition == EURGPartCondition::Damaged;

	MovementComponent->MaxWalkSpeed = MoveSpeed * Stats.MoveSpeedScale * ConditionScale;
	JumpMaxCount = bDamaged ? 1 : Stats.JumpMaxCount;
	bDashEnabled = Stats.bCanDash;
	DashSpeedScale = Stats.DashSpeedScale * ConditionScale;
	DashCooldownScale = Stats.DashCooldownScale;

	// 대시가 없는 다리로 바꾸면 이전 다리의 쿨다운 표시가 남지 않게 한다
	if (!bDashEnabled)
	{
		OnDashCooldownChanged.Broadcast(0.0f, 0.0f);
	}
}

void AURGPlayerCharacter::NotifyCoreAbilityActivated(const FText& InMessage)
{
	OnCoreAbilityActivated.Broadcast(InMessage);
}

void AURGPlayerCharacter::NotifyCoreAbilityStateChanged(const bool bInActive, const float InCooldownEndTime, const float InCooldownDuration)
{
	OnCoreAbilityStateChanged.Broadcast(bInActive, InCooldownEndTime, InCooldownDuration);
}

void AURGPlayerCharacter::ApplyCorePart()
{
	if (PartComponent == nullptr)
	{
		return;
	}

	const bool bUsable = PartComponent->IsSlotUsable(EURGPartSlot::Core);
	const TSubclassOf<UURGCoreAbility> AbilityClass = bUsable ? PartComponent->GetSlotState(EURGPartSlot::Core).Part->GetCoreAbilityClass() : nullptr;

	// 클래스가 달라지면(교체·제거·파손) 진행 중인 효과를 끊고 새로 만든다. 손상 단계만 바뀐 경우 쿨다운은 유지
	if (CoreAbility == nullptr || CoreAbility->GetClass() != AbilityClass.Get())
	{
		if (CoreAbility)
		{
			CoreAbility->Cancel();
			CoreAbility = nullptr;
		}

		if (AbilityClass)
		{
			CoreAbility = NewObject<UURGCoreAbility>(this, AbilityClass);
			CoreAbility->Initialize(this);
		}

		// 새 능력은 쿨다운 없이 시작하고, 능력이 없어진 경우도 표시를 비워야 한다
		NotifyCoreAbilityStateChanged(false, 0.0f, 0.0f);
	}

	// 손상(Damaged)이면 쿨다운이 길어진다
	if (CoreAbility)
	{
		CoreAbility->SetCooldownScale(1.0f / PartComponent->GetConditionScale(EURGPartSlot::Core));
	}
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
		if (RadarComponent)
		{
			RadarComponent->SetRadarEnabled(false);
		}
		return;
	}

	// 손상(Damaged)이면 치명타 확률이 깎인다. 배율은 그대로
	const FURGHeadStats& Stats = PartComponent->GetSlotState(EURGPartSlot::Head).Part->GetHeadStats();
	CombatComponent->SetCritStats(Stats.CritChance * PartComponent->GetConditionScale(EURGPartSlot::Head), Stats.CritDamageScale);

	// 적 탐지는 파손 전까지 동작한다
	if (RadarComponent)
	{
		RadarComponent->SetRadarEnabled(Stats.bRevealEnemies);
	}
}

void AURGPlayerCharacter::ApplySynergies()
{
	if (PartComponent == nullptr)
	{
		return;
	}

	// 강화: 최대 체력 보너스는 켜지고 꺼지는 순간에만 더하고 뺀다 (매번 더하면 슬롯이 바뀔 때마다 쌓인다)
	const bool bReinforce = PartComponent->IsSynergyActive(URGGameplayTags::Part_Tag_Reinforce);
	if (AttributeComponent && bReinforce != bReinforceApplied)
	{
		AttributeComponent->AddMaxHealth(bReinforce ? ReinforceMaxHealthBonus : -ReinforceMaxHealthBonus);
		bReinforceApplied = bReinforce;
	}

	PartComponent->SetPartDamageScale(bReinforce ? ReinforcePartDamageScale : 1.0f);

	// 열·전기: 적중 효과는 무기가 데미지를 낼 때 CombatComponent가 붙인다
	const bool bThermal = PartComponent->IsSynergyActive(URGGameplayTags::Part_Tag_Thermal);
	const bool bElectric = PartComponent->IsSynergyActive(URGGameplayTags::Part_Tag_Electric);
	if (CombatComponent)
	{
		CombatComponent->SetSynergyEffects(bThermal, bElectric);
	}

	UpdateSynergyState(URGGameplayTags::Part_Tag_Reinforce, bReinforce);
	UpdateSynergyState(URGGameplayTags::Part_Tag_Thermal, bThermal);
	UpdateSynergyState(URGGameplayTags::Part_Tag_Electric, bElectric);
}

void AURGPlayerCharacter::UpdateSynergyState(const FGameplayTag& InTag, const bool bInActive)
{
	// 슬롯이 바뀔 때마다 부르므로 실제로 켜지고 꺼진 순간만 알린다 (HUD 문구가 반복되지 않도록)
	if (ActiveSynergies.HasTagExact(InTag) == bInActive)
	{
		return;
	}

	if (bInActive)
	{
		ActiveSynergies.AddTag(InTag);
	}
	else
	{
		ActiveSynergies.RemoveTag(InTag);
	}

	OnSynergyChanged.Broadcast(InTag, bInActive);
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

bool AURGPlayerCharacter::ResolvePartSlotFromHit(const FHitResult& InHit, EURGPartSlot& OutSlot) const
{
	FName Bone = InHit.BoneName;

	// 캡슐에 맞으면 뼈 이름이 비어 오므로 충돌점에서 가장 가까운 바디 뼈로 대신한다.
	if (Bone.IsNone())
	{
		Bone = GetMesh()->FindClosestBone(InHit.ImpactPoint, nullptr, 0.0f, /*bRequirePhysicalAsset*/ true);
	}

	while (!Bone.IsNone())
	{
		if (const EURGPartSlot* Found = HitBoneSlotMap.Find(Bone))
		{
			OutSlot = *Found;
			return true;
		}

		Bone = GetMesh()->GetParentBone(Bone);
	}

	return false;
}

void AURGPlayerCharacter::HandleDamaged(float InDamage, AActor* InCauser)
{	// 사망 처리는 HandleDeath가, 대시 중에는 데미지만 받고 리액션은 생략한다.
	if (IsDead() || StateComponent->IsCurrentStateEqualTo(URGGameplayTags::Character_State_Dashing))
	{
		return;
	}

	// 피격은 재장전을 끊는다 (재장전 중이 아니면 아무 일도 하지 않는다).
	CancelReload();

	// 리액션 몽타주가 없는 방향으로 맞아도 소리는 나야 하므로 몽타주 검사보다 먼저 재생한다
	PlayPainSound(InDamage);

	// 가해자를 모르면 정면 피격으로 본다.
	const EURGHitDirection Direction = InCauser ? CalcHitDirection(InCauser->GetActorLocation()) : EURGHitDirection::Front;

	// 몽타주가 없는 방향은 리액션도 상태 진입도 생략한다 (풀어 줄 계기가 없으므로).
	UAnimMontage* Montage = HitReactMontages.FindRef(Direction);
	if (Montage == nullptr)
	{
		return;
	}

	StateComponent->SetCurrentState(URGGameplayTags::Character_State_Hit);
	PlayAnimMontage(Montage);

	// 상태 해제는 몽타주 종료 델리게이트로 맞춘다 (애셋 노티파이에 의존하지 않는다).
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AURGPlayerCharacter::HandleHitReactEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
	}
}

void AURGPlayerCharacter::PlayPainSound(float InDamage)
{
	const float MaxHealth = AttributeComponent ? AttributeComponent->GetMaxHealth() : 0.0f;
	const bool bHeavy = MaxHealth > 0.0f && InDamage >= MaxHealth * PainHeavyDamageRatio;
	const float Now = GetWorld()->GetTimeSeconds();

	// 큰 피해는 확실히 알려야 하므로 간격 제한을 무시한다
	if (!bHeavy && Now - LastPainSoundTime < PainSoundInterval)
	{
		return;
	}

	USoundBase* Sound = bHeavy ? PainHeavySound.Get() : PainSound.Get();
	if (Sound == nullptr)
	{
		return;
	}

	// 메시에 붙여 캐릭터가 밀려나도 소리가 몸을 따라가게 한다
	UGameplayStatics::SpawnSoundAttached(Sound, GetMesh());
	LastPainSoundTime = Now;
}

void AURGPlayerCharacter::HandleHitReactEnded(UAnimMontage* InMontage, bool bInterrupted)
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
	Damage *= IncomingDamageScale;
	if (Damage <= 0.0f)
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage <= 0.0f || IsDead() || PartComponent == nullptr)
	{
		return ActualDamage;
	}

	// 위치 없는 데미지(근접·폭발)는 체력만 깎인다.
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PointEvent = static_cast<const FPointDamageEvent&>(DamageEvent);

		EURGPartSlot Slot;
		if (ResolvePartSlotFromHit(PointEvent.HitInfo, Slot))
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

bool AURGPlayerCharacter::IsAiming() const
{
	return AimComponent && AimComponent->IsAiming();
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

bool AURGPlayerCharacter::CanDash() const
{
	if (IsActionBlocked())
	{
		return false;
	}

	// 대시는 다리 부품이 제공한다. 빈 슬롯·파손·대시 없는 다리면 불가 (ApplyLegPart가 갱신)
	if (!bDashEnabled)
	{
		return false;
	}

	// 쿨다운
	const float Now = GetWorld()->GetTimeSeconds();
	return (Now - LastDashTime) >= DashCooldown * DashCooldownScale;
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

	const AURGWeapon* Weapon = GetMainWeapon();
	return Weapon && Weapon->CanReload();
}

void AURGPlayerCharacter::HandleDeath()
{
	CancelReload();

	// 조준 중에 죽으면 줌 FOV와 조준 회전이 남아 래그돌 몸통을 계속 돌리므로 평상시로 되돌린다
	if (AimComponent)
	{
		AimComponent->StopAim();
	}

	// 죽은 뒤에는 상호작용할 수 없으므로 비교 UI 등이 남지 않게 포커스를 비운다
	GetWorldTimerManager().ClearTimer(InteractFocusTimer);
	SetInteractFocus(nullptr);

	// 적 마커도 지운다
	if (RadarComponent)
	{
		RadarComponent->SetRadarEnabled(false);
	}

	// 사망한 판의 상태가 다음 판으로 새어 나가지 않게 보관본을 버린다.
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UPlayerStatSubsystem* StatSubsystem = GameInstance->GetSubsystem<UPlayerStatSubsystem>())
		{
			StatSubsystem->ClearSnapshot();
		}
	}

	// 시간 감속 같은 지속 효과가 사망 뒤에도 남지 않게 끊는다
	if (CoreAbility)
	{
		CoreAbility->Cancel();
	}

	// 공격·피격 몽타주를 끊는다.
	StopAnimMontage();

	// 메시에 붙여 래그돌로 날아가도 비명이 몸을 따라가게 한다
	if (DeathSound)
	{
		UGameplayStatics::SpawnSoundAttached(DeathSound, GetMesh());
	}

	Super::HandleDeath();

	// Death 애님은 튕겨 오르는 구간까지만 있어(래그돌 인계용) 물리로 넘겨 바닥에 눕힌다.
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (MeshComp == nullptr)
	{
		return;
	}

	MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
	MeshComp->SetSimulatePhysics(true);

	// 마지막 타격 방향으로 밀려 나가며 쓰러지게 한다. 가해자를 모르면 제자리에서 쓰러진다.
	const AActor* Causer = AttributeComponent ? AttributeComponent->GetLastDamageCauser() : nullptr;
	if (Causer == nullptr)
	{
		return;
	}

	FVector PushDirection = (GetActorLocation() - Causer->GetActorLocation()).GetSafeNormal2D();
	PushDirection = (PushDirection + FVector::UpVector * DeathImpulseUpRatio).GetSafeNormal();
	// 래그돌은 바디가 여러 개라 하나만 밀면 나머지에 붙잡힌다. 전체 바디에 같은 속도 변화를 준다.
	MeshComp->AddImpulseToAllBodiesBelow(PushDirection * DeathImpulse, NAME_None, true); // bVelChange: 질량과 무관
}

void AURGPlayerCharacter::TryAttack(const FGameplayTag& InAttackType)
{
	if (!CanAttack())
	{
		return;
	}

	AURGWeapon* Weapon = GetMainWeapon();
	if (Weapon == nullptr)
	{
		return;
	}

	// 무기가 공격할 수 없는 상태(탄약 없음)면 공격 대신 자동 재장전
	if (!Weapon->CanAttack())
	{
		TryReload();
		return;
	}

	// 이전 공격 모션이 끝나기 전에는 다음 공격을 받지 않는다 (모션이 끊기지 않도록)
	if (IsAttackMontagePlaying())
	{
		return;
	}

	// 공격 간격 제어: 무기 쿨다운과 연사 최소 간격 중 긴 쪽. 시각은 캐릭터가 기록하므로 무기를 바꿔도 쿨다운을 건너뛸 수 없다
	const float Now = GetWorld()->GetTimeSeconds();
	const float RequiredInterval = FMath::Max(RapidFireInterval, Weapon->GetAttackCooldown());
	if (LastFireTime >= 0.0f && Now - LastFireTime < RequiredInterval)
	{
		return;
	}

	LastFireTime = Now;

	Weapon->Attack(InAttackType);

	if (UAnimMontage* AttackMontage = Weapon->GetAttackMontage())
	{
		PlayAnimMontage(AttackMontage);
	}

	BroadcastAttackCooldown();
}

void AURGPlayerCharacter::BroadcastAttackCooldown() const
{
	const AURGWeapon* Weapon = GetMainWeapon();
	const float Cooldown = Weapon ? Weapon->GetAttackCooldown() : 0.0f;

	if (LastFireTime < 0.0f || Cooldown < AttackCooldownDisplayMin)
	{
		OnAttackCooldownChanged.Broadcast(0.0f, 0.0f);
		return;
	}

	// 끝나는 시각이 이미 지났으면 위젯이 알아서 표시를 끈다
	OnAttackCooldownChanged.Broadcast(LastFireTime + Cooldown, Cooldown);
}

bool AURGPlayerCharacter::IsAttackMontagePlaying() const
{
	const AURGWeapon* Weapon = GetMainWeapon();
	const UAnimMontage* AttackMontage = Weapon ? Weapon->GetAttackMontage() : nullptr;
	if (AttackMontage == nullptr || GetMesh() == nullptr)
	{
		return false;
	}

	const UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance == nullptr || AnimInstance->Montage_IsPlaying(AttackMontage) == false)
	{
		return false;
	}

	// 블렌드 아웃 구간에 들어가면 다음 공격을 허용한다 (모션이 자연스럽게 이어지도록)
	const float BlendOutStart = AttackMontage->GetPlayLength() - AttackMontage->GetDefaultBlendOutTime();
	return AnimInstance->Montage_GetPosition(AttackMontage) < BlendOutStart;
}

void AURGPlayerCharacter::TryReload()
{
	if (!CanReload())
	{
		return;
	}

	check(StateComponent);

	// 재장전 동안 일반행동 상태(발사/대시 차단). 이동은 허용한다.
	StateComponent->SetCurrentState(URGGameplayTags::Character_State_GeneralAction);

	// 무기에 재장전 몽타주가 있으면 그 길이만큼, 없으면 ReloadDuration만큼 대기 후 탄약을 채운다
	float Duration = ReloadDuration;
	if (const AURGWeapon* Weapon = GetMainWeapon())
	{
		if (UAnimMontage* ReloadMontage = Weapon->GetReloadMontage())
		{
			const float MontageDuration = PlayAnimMontage(ReloadMontage);
			if (MontageDuration > 0.0f)
			{
				Duration = MontageDuration;
			}
		}
	}

	GetWorldTimerManager().SetTimer(ReloadTimer, this, &AURGPlayerCharacter::FinishReload, Duration, false);
}

void AURGPlayerCharacter::FinishReload()
{
	if (AURGWeapon* Weapon = GetMainWeapon())
	{
		Weapon->Reload();
	}

	ClearReloadState();
}

void AURGPlayerCharacter::CancelReload()
{
	if (!GetWorldTimerManager().IsTimerActive(ReloadTimer))
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ReloadTimer);

	// 재생 중인 재장전 몽타주도 끊는다
	const AURGWeapon* Weapon = GetMainWeapon();
	if (UAnimMontage* ReloadMontage = Weapon ? Weapon->GetReloadMontage() : nullptr)
	{
		StopAnimMontage(ReloadMontage);
	}

	ClearReloadState();
}

void AURGPlayerCharacter::ClearReloadState()
{
	// 재장전 중 다른 상태(피격 등)로 덮였으면 그 상태는 건드리지 않는다.
	if (StateComponent && StateComponent->IsCurrentStateEqualTo(URGGameplayTags::Character_State_GeneralAction))
	{
		StateComponent->ClearCurrentState();
	}
}

void AURGPlayerCharacter::Input_Move(const FInputActionValue& InputValue)
{
	// 사망/이동 입력 잠금 상태에서는 이동 불가
	if (IsDead())
	{
		return;
	}

	if (StateComponent && StateComponent->MovementInputEnabled() == false)
	{
		return;
	}

	const FVector2D MovementVector = InputValue.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardVector, MovementVector.X);
		AddMovementInput(RightVector, MovementVector.Y);
	}
}

void AURGPlayerCharacter::Input_Look(const FInputActionValue& InputValue)
{
	const FVector2D LookVector = InputValue.Get<FVector2D>();

	AddControllerYawInput(LookVector.X);
	AddControllerPitchInput(LookVector.Y);
}

void AURGPlayerCharacter::Input_Jump(const FInputActionValue& InputValue)
{
	if (IsDead())
	{
		return;
	}

	if (StateComponent && StateComponent->MovementInputEnabled() == false)
	{
		return;
	}

	if (InputValue.Get<bool>())
	{
		Jump();
	}
}

void AURGPlayerCharacter::Input_StopJump(const FInputActionValue& InputValue)
{
	if (!InputValue.Get<bool>())
	{
		StopJumping();
	}
}

void AURGPlayerCharacter::Input_Dash()
{
	if (!CanDash())
	{
		return;
	}

	// 대시 방향: 이번 프레임 입력 → 지난 프레임 입력 → 캐릭터 전방 순으로 폴백
	FVector Direction = GetPendingMovementInputVector();
	if (Direction.IsNearlyZero())
	{
		Direction = GetLastMovementInputVector();
	}
	if (Direction.IsNearlyZero())
	{
		Direction = GetActorForwardVector();
	}

	Direction.Z = 0.0f;
	Direction.Normalize();

	LastDashTime = GetWorld()->GetTimeSeconds();

	// CanDash와 같은 기준(월드 초)으로 끝나는 시각을 넘겨, 위젯이 남은 시간을 직접 계산하게 한다
	const float CooldownDuration = DashCooldown * DashCooldownScale;
	OnDashCooldownChanged.Broadcast(LastDashTime + CooldownDuration, CooldownDuration);

	check(StateComponent);
	StateComponent->SetCurrentState(URGGameplayTags::Character_State_Dashing);

	// 대시 동안 이동 입력을 잠그고, 종료 시 입력 복구 + 상태 클리어
	StateComponent->ToggleMovementInput(false);
	StateComponent->ToggleMovementInput(true, DashDuration);

	// 수평 속도만 덮어쓴다 (공중 대시 시 낙하 속도는 유지)
	LaunchCharacter(Direction * DashSpeed * DashSpeedScale, true, false);
}

void AURGPlayerCharacter::Input_SingleAttack()
{
	TryAttack(URGGameplayTags::Character_Attack_Single);
}

void AURGPlayerCharacter::Input_RapidAttack()
{
	TryAttack(URGGameplayTags::Character_Attack_Rapid);
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

	if (AActor* Target = FindInteractTarget())
	{
		IURGInteractable::Execute_Interact(Target, this);
	}
}

void AURGPlayerCharacter::UpdateInteractFocus()
{
	SetInteractFocus(IsDead() ? nullptr : FindInteractTarget());
}

void AURGPlayerCharacter::SetInteractFocus(AActor* InActor)
{
	// 같은 대상을 계속 보고 있으면 알리지 않는다 (HUD가 0.1초마다 다시 그리지 않도록)
	if (InteractFocus.Get() == InActor)
	{
		return;
	}

	InteractFocus = InActor;
	OnInteractFocusChanged.Broadcast(InActor);
}

AActor* AURGPlayerCharacter::FindInteractTarget()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	// 카메라 시점에서 전방으로 InteractTrace. 카메라는 캐릭터 뒤에 있으므로 그 거리만큼 사거리를 보정한다.
	FVector ViewLocation = GetActorLocation();
	FRotator ViewRotation = GetActorRotation();
	if (Controller)
	{
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}

	const float CameraOffset = FVector::Dist(ViewLocation, GetActorLocation());
	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + ViewRotation.Vector() * (InteractRange + CameraOffset);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(Interact), false);
	Params.AddIgnoredActor(this);

	if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_InteractTrace, Params))
	{
		return nullptr;
	}

	AActor* HitActor = Hit.GetActor();
	if (HitActor == nullptr || HitActor->Implements<UURGInteractable>() == false)
	{
		return nullptr;
	}

	return IURGInteractable::Execute_CanInteract(HitActor, this) ? HitActor : nullptr;
}

void AURGPlayerCharacter::Input_CoreAbility()
{
	if (IsDead())
	{
		return;
	}

	// 코어 부품이 없거나 파손이면 ApplyCorePart가 능력을 비워 둔다
	if (CoreAbility)
	{
		CoreAbility->TryActivate();
	}
}

void AURGPlayerCharacter::Input_Pause()
{
	// 일시정지 화면은 컨트롤러 델리게이트를 구독하는 UI 담당이 처리한다.
	if (AURGPlayerController* PlayerController = Cast<AURGPlayerController>(GetController()))
	{
		PlayerController->RequestPause();
	}
}



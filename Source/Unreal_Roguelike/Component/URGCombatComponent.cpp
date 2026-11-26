#include "Component/URGCombatComponent.h"
#include "Characters/URGCharacter.h"
#include "Component/URGBurnComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Equipment/URGWeapon.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	/** 적중 효과를 걸 수 있는 대상인지 (살아 있는 캐릭터이고 소유자 자신이 아님) */
	bool IsValidEffectTarget(const AActor* InTarget, const AActor* InOwner)
	{
		const AURGCharacter* TargetCharacter = Cast<AURGCharacter>(InTarget);
		return TargetCharacter != nullptr && TargetCharacter != InOwner && !TargetCharacter->IsDead();
	}
}

UURGCombatComponent::UURGCombatComponent()
{
	// 공격·재장전 입력과 타이머로만 동작하므로 매 프레임 Tick은 필요하지 않다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UURGCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	check(OwnerCharacter);
}

void UURGCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 소유자가 사라지는 중이므로 재장전 종료를 알리지 않고 타이머만 끈다
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReloadTimer);
	}

	// 소유자가 사라질 때 스폰해 둔 무기 액터도 함께 정리
	UnequipWeapon();

	Super::EndPlay(EndPlayReason);
}

void UURGCombatComponent::EquipWeapon(const TSubclassOf<AURGWeapon> InWeaponClass)
{
	// 이전 무기에 걸려 있던 재장전은 새 무기에 적용되면 안 된다
	CancelReload();
	UnequipWeapon();
	SpawnMainWeapon(InWeaponClass);

	// 쿨다운은 마지막 공격 시각 기준이므로 새 무기의 쿨다운 길이로 남은 시간을 다시 계산해 보여 준다
	BroadcastAttackCooldown();
}

bool UURGCombatComponent::NeedsReload() const
{
	return MainWeapon != nullptr && !MainWeapon->CanAttack();
}

void UURGCombatComponent::TryAttack()
{
	if (MainWeapon == nullptr)
	{
		return;
	}

	// 이전 공격 모션이 끝나기 전에는 다음 공격을 받지 않는다 (모션이 끊기지 않도록)
	if (IsAttackMontagePlaying())
	{
		return;
	}

	// 공격 간격: 무기 쿨다운과 연사 최소 간격 중 긴 쪽
	const float Now = GetWorld()->GetTimeSeconds();
	const float RequiredInterval = FMath::Max(RapidFireInterval, MainWeapon->GetAttackCooldown());
	if (LastFireTime >= 0.0f && Now - LastFireTime < RequiredInterval)
	{
		return;
	}

	LastFireTime = Now;

	MainWeapon->Attack();

	if (UAnimMontage* AttackMontage = MainWeapon->GetAttackMontage())
	{
		OwnerCharacter->PlayAnimMontage(AttackMontage);
	}

	BroadcastAttackCooldown();
}

bool UURGCombatComponent::CanReload() const
{
	return MainWeapon != nullptr && MainWeapon->CanReload();
}

void UURGCombatComponent::StartReload()
{
	// 무기에 재장전 몽타주가 있으면 그 길이만큼, 없으면 ReloadDuration만큼 기다린다
	float Duration = ReloadDuration;
	UAnimMontage* ReloadMontage = MainWeapon != nullptr ? MainWeapon->GetReloadMontage() : nullptr;
	if (ReloadMontage != nullptr)
	{
		const float MontageDuration = OwnerCharacter->PlayAnimMontage(ReloadMontage);
		if (MontageDuration > 0.0f)
		{
			Duration = MontageDuration;
		}
	}

	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &UURGCombatComponent::HandleReloadTimerExpired, Duration, false);
}

void UURGCombatComponent::CancelReload()
{
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	if (!TimerManager.IsTimerActive(ReloadTimer))
	{
		return;
	}

	TimerManager.ClearTimer(ReloadTimer);

	// 재생 중인 재장전 몽타주도 끊는다
	UAnimMontage* ReloadMontage = MainWeapon != nullptr ? MainWeapon->GetReloadMontage() : nullptr;
	if (ReloadMontage != nullptr)
	{
		OwnerCharacter->StopAnimMontage(ReloadMontage);
	}

	// 취소도 알려야 소유자가 일반행동 상태를 푼다 (피격 리액션이 없는 방향이면 다른 상태가 덮지 않는다)
	OnReloadEnded.Broadcast(false);
}

void UURGCombatComponent::HandleReloadTimerExpired()
{
	if (MainWeapon != nullptr)
	{
		MainWeapon->Reload();
	}

	OnReloadEnded.Broadcast(true);
}

bool UURGCombatComponent::IsAttackMontagePlaying() const
{
	const UAnimMontage* AttackMontage = MainWeapon != nullptr ? MainWeapon->GetAttackMontage() : nullptr;
	if (AttackMontage == nullptr)
	{
		return false;
	}

	const USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	const UAnimInstance* AnimInstance = Mesh != nullptr ? Mesh->GetAnimInstance() : nullptr;
	if (AnimInstance == nullptr || !AnimInstance->Montage_IsPlaying(AttackMontage))
	{
		return false;
	}

	// 블렌드 아웃 구간에 들어가면 다음 공격을 허용한다 (모션이 자연스럽게 이어지도록)
	const float BlendOutStart = AttackMontage->GetPlayLength() - AttackMontage->GetDefaultBlendOutTime();
	return AnimInstance->Montage_GetPosition(AttackMontage) < BlendOutStart;
}

void UURGCombatComponent::BroadcastAttackCooldown() const
{
	const float Cooldown = MainWeapon != nullptr ? MainWeapon->GetAttackCooldown() : 0.0f;

	if (LastFireTime < 0.0f || Cooldown < AttackCooldownDisplayMin)
	{
		OnAttackCooldownChanged.Broadcast(0.0f, 0.0f);
		return;
	}

	// 끝나는 시각이 이미 지났으면 위젯이 알아서 표시를 끈다
	OnAttackCooldownChanged.Broadcast(LastFireTime + Cooldown, Cooldown);
}

void UURGCombatComponent::SpawnMainWeapon(const TSubclassOf<AURGWeapon> InWeaponClass)
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (InWeaponClass == nullptr || OwnerActor == nullptr || World == nullptr)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerActor;
	SpawnParams.Instigator = Cast<APawn>(OwnerActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Owner를 지정해 스폰해야 무기가 소유 캐릭터 메시에 부착되고 데미지 주체를 알 수 있다.
	AURGWeapon* SpawnedWeapon = World->SpawnActor<AURGWeapon>(InWeaponClass, OwnerActor->GetActorTransform(), SpawnParams);
	if (SpawnedWeapon == nullptr)
	{
		return;
	}

	SetMainWeapon(SpawnedWeapon);
	SpawnedWeapon->EquipItem();
}

void UURGCombatComponent::SetCritStats(float InChance, float InDamageScale)
{
	CritChance = FMath::Clamp(InChance, 0.0f, 1.0f);
	CritDamageScale = FMath::Max(InDamageScale, 1.0f);
}

float UURGCombatComponent::ModifyOutgoingDamage(float InDamage, bool& bOutCrit) const
{
	float Damage = InDamage * AttackDamageScale;

	// 치명타는 호출마다 굴린다 (한 발·한 타격 단위)
	bOutCrit = CritChance > 0.0f && FMath::FRand() < CritChance;
	if (bOutCrit)
	{
		Damage *= CritDamageScale;
	}

	return Damage;
}

void UURGCombatComponent::NotifyHitDealt(const FVector& InLocation, float InDamage, bool bInCrit, bool bInKilled) const
{
	OnHitDealt.Broadcast(InLocation, InDamage, bInCrit, bInKilled);
}

void UURGCombatComponent::SetSynergyEffects(bool bInBurn, bool bInChain)
{
	bBurnEnabled = bInBurn;
	bChainEnabled = bInChain;
}

void UURGCombatComponent::ApplyOnHitEffects(AActor* InTarget, const FVector& InHitLocation, float InDamage) const
{
	if (!bBurnEnabled && !bChainEnabled)
	{
		return;
	}

	// 벽·소품에 맞은 경우와 이번 한 방에 죽은 대상은 제외한다
	if (!IsValidEffectTarget(InTarget, GetOwner()))
	{
		return;
	}

	if (bBurnEnabled)
	{
		ApplyBurn(InTarget);
	}

	if (bChainEnabled)
	{
		ApplyChain(InTarget, InHitLocation, InDamage);
	}
}

void UURGCombatComponent::ApplyBurn(AActor* InTarget) const
{
	// 적 클래스를 건드리지 않고 효과를 붙이기 위해 대상에 런타임 컴포넌트를 단다 (적중 시에만 한 번 찾는다)
	UURGBurnComponent* Burn = InTarget->FindComponentByClass<UURGBurnComponent>();
	if (Burn == nullptr)
	{
		Burn = NewObject<UURGBurnComponent>(InTarget);
		Burn->RegisterComponent();
	}

	Burn->StartBurn(GetOwner(), BurnDamagePerTick, BurnTickInterval, BurnDuration);
}

void UURGCombatComponent::ApplyChain(const AActor* InTarget, const FVector& InCenter, float InDamage) const
{
	UWorld* World = GetWorld();
	AActor* OwnerActor = GetOwner();
	const float ChainDamage = InDamage * ChainDamageRatio;
	if (World == nullptr || OwnerActor == nullptr || ChainMaxTargets <= 0 || ChainDamage <= 0.0f)
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(URGChainOverlap), false, OwnerActor);
	QueryParams.AddIgnoredActor(InTarget);

	// 적 콜리전 프로필에 기대지 않도록 동적 오브젝트 전부를 훑고 캐릭터만 남긴다
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, InCenter, FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllDynamicObjects),
		FCollisionShape::MakeSphere(ChainRadius), QueryParams);

	// 한 캐릭터가 캡슐·메시 등 여러 컴포넌트로 잡히므로 액터 단위로 모은다
	TArray<AActor*> Candidates;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (IsValidEffectTarget(Candidate, OwnerActor))
		{
			Candidates.AddUnique(Candidate);
		}
	}

	Candidates.Sort([&InCenter](const AActor& A, const AActor& B)
	{
		return FVector::DistSquared(A.GetActorLocation(), InCenter) < FVector::DistSquared(B.GetActorLocation(), InCenter);
	});

	const APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	AController* InstigatorController = OwnerPawn != nullptr ? OwnerPawn->GetController() : nullptr;

	const int32 NumTargets = FMath::Min(ChainMaxTargets, Candidates.Num());
	for (int32 Index = 0; Index < NumTargets; ++Index)
	{
		// 위치 없는 데미지로 넣는다. 무기를 거치지 않으므로 전이가 다시 전이를 부르지 않는다
		UGameplayStatics::ApplyDamage(Candidates[Index], ChainDamage, InstigatorController, OwnerActor, nullptr);
	}
}

void UURGCombatComponent::UnequipWeapon()
{
	if (MainWeapon == nullptr)
	{
		return;
	}

	AURGWeapon* Weapon = MainWeapon;

	Weapon->UnequipItem();
	SetMainWeapon(nullptr);
	Weapon->Destroy();
}

void UURGCombatComponent::SetMainWeapon(AURGWeapon* InNewWeapon)
{
	MainWeapon = InNewWeapon;

	OnWeaponChanged.Broadcast();
}


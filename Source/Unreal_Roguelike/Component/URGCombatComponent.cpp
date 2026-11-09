#include "Component/URGCombatComponent.h"
#include "Characters/URGCharacter.h"
#include "Component/URGBurnComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Equipment/URGWeapon.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** 적중 효과를 걸 수 있는 대상인지 (살아 있는 캐릭터이고 소유자 자신이 아님) */
	bool IsValidEffectTarget(const AActor* InTarget, const AActor* InOwner)
	{
		const AURGCharacter* TargetCharacter = Cast<AURGCharacter>(InTarget);
		return TargetCharacter && TargetCharacter != InOwner && !TargetCharacter->IsDead();
	}
}

UURGCombatComponent::UURGCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UURGCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 소유자가 사라질 때 스폰해 둔 무기 액터도 함께 정리
	UnequipWeapon();

	Super::EndPlay(EndPlayReason);
}

void UURGCombatComponent::EquipWeapon(const TSubclassOf<AURGWeapon> InWeaponClass)
{
	UnequipWeapon();

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

void UURGCombatComponent::SetCritStats(const float InChance, const float InDamageScale)
{
	CritChance = FMath::Clamp(InChance, 0.0f, 1.0f);
	CritDamageScale = FMath::Max(InDamageScale, 1.0f);
}

float UURGCombatComponent::ModifyOutgoingDamage(const float InDamage, bool& bOutCrit) const
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

void UURGCombatComponent::NotifyHitDealt(const FVector& InLocation, const float InDamage, const bool bInCrit, const bool bInKilled) const
{
	OnHitDealt.Broadcast(InLocation, InDamage, bInCrit, bInKilled);
}

void UURGCombatComponent::SetSynergyEffects(const bool bInBurn, const bool bInChain)
{
	bBurnEnabled = bInBurn;
	bChainEnabled = bInChain;
}

void UURGCombatComponent::ApplyOnHitEffects(AActor* InTarget, const FVector& InHitLocation, const float InDamage) const
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

void UURGCombatComponent::ApplyChain(const AActor* InTarget, const FVector& InCenter, const float InDamage) const
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
	AController* InstigatorController = OwnerPawn ? OwnerPawn->GetController() : nullptr;

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


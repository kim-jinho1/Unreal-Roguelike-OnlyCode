#include "Equipment/URGWeapon.h"
#include "Characters/URGCharacter.h"
#include "Component/URGAttributeComponent.h"
#include "Component/URGCombatComponent.h"
#include "Equipment/URGWeaponCameraShake.h"
#include "URGGameplayTags.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AURGWeapon::AURGWeapon()
{
	// 기본 셰이크 (BP에서 교체 가능)
	CameraShakeClass = UURGWeaponCameraShake::StaticClass();
}

void AURGWeapon::EquipItem()
{
	Super::EquipItem();

	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	// 소유자의 전투 컴포넌트를 장착 시점에 한 번만 캐싱 (공격력 배율 조회용)
	CombatComponent = OwnerActor->GetComponentByClass<UURGCombatComponent>();
	AttachToOwner(EquipSocketName);
}

void AURGWeapon::UnequipItem()
{
	Super::UnequipItem();

	CombatComponent = nullptr;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

void AURGWeapon::Attack(const FGameplayTag InAttackType)
{
	CurrentAttackType = InAttackType;
}

bool AURGWeapon::CanAttack() const
{
	return true;
}

UAnimMontage* AURGWeapon::GetReloadMontage() const
{
	return nullptr;
}

bool AURGWeapon::CanReload() const
{
	return false;
}

void AURGWeapon::Reload()
{
}

float AURGWeapon::RollAttackDamage(bool& bOutCrit) const
{
	bOutCrit = false;
	float Damage = BaseDamage;

	if (CurrentAttackType == URGGameplayTags::Character_Attack_Rapid)
	{
		Damage *= RapidDamageScale;
	}

	// 팔 부품 배율과 머리 부품 치명타는 CombatComponent가 한 번에 적용한다
	if (CombatComponent)
	{
		Damage = CombatComponent->ModifyOutgoingDamage(Damage, bOutCrit);
	}

	return Damage;
}

FVector AURGWeapon::GetSocketLocationOrOwner(const FName InSocketName) const
{
	// 1) 무기 메시 소켓
	if (Mesh && Mesh->GetStaticMesh() && Mesh->DoesSocketExist(InSocketName))
	{
		return Mesh->GetSocketLocation(InSocketName);
	}

	// 2) 소유 캐릭터 메시 소켓 (캐릭터 메시에 포가 달린 경우)
	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		const USkeletalMeshComponent* CharacterMesh = OwnerCharacter->GetMesh();
		if (CharacterMesh && CharacterMesh->DoesSocketExist(InSocketName))
		{
			return CharacterMesh->GetSocketLocation(InSocketName);
		}
	}

	// 3) 액터 위치
	return GetActorLocation();
}

void AURGWeapon::GetOwnerViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	const AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		OutLocation = GetActorLocation();
		OutRotation = GetActorRotation();
		return;
	}

	OutLocation = OwnerActor->GetActorLocation();
	OutRotation = OwnerActor->GetActorRotation();

	// 컨트롤러가 있으면 카메라 시점을 우선한다
	if (const APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		if (AController* OwnerController = OwnerPawn->GetController())
		{
			OwnerController->GetPlayerViewPoint(OutLocation, OutRotation);
		}
	}
}

bool AURGWeapon::ApplyDamageToHit(const FHitResult& InHit, const FVector& InDamageDirection, const float InDamage, const bool bInCrit)
{
	AActor* TargetActor = InHit.GetActor();
	AActor* OwnerActor = GetOwner();
	if (TargetActor == nullptr || OwnerActor == nullptr)
	{
		return false;
	}

	// 적중 효과(전기 전이)가 같은 값을 쓰도록 데미지를 한 번만 굴린다
	bool bCrit = bInCrit;
	const float Damage = InDamage > 0.0f ? InDamage : RollAttackDamage(bCrit);

	const float HealthBefore = GetAliveHealth(TargetActor);

	UGameplayStatics::ApplyPointDamage(
		TargetActor,
		Damage,
		InDamageDirection,
		InHit,
		OwnerActor->GetInstigatorController(),
		this,
		nullptr);

	const bool bLanded = ReportHit(TargetActor, HealthBefore, InHit.ImpactPoint, bCrit);

	// 태그 시너지 적중 효과 (화상·전이). 데미지 뒤에 걸어야 이번 한 방에 죽은 대상을 거를 수 있다
	if (CombatComponent)
	{
		CombatComponent->ApplyOnHitEffects(TargetActor, InHit.ImpactPoint, Damage);
	}

	return bLanded;
}

float AURGWeapon::GetAliveHealth(const AActor* InTarget)
{
	const AURGCharacter* TargetCharacter = Cast<AURGCharacter>(InTarget);
	const UURGAttributeComponent* Attribute = TargetCharacter ? TargetCharacter->GetAttributeComponent() : nullptr;
	if (Attribute == nullptr || !Attribute->IsAlive())
	{
		return -1.0f;
	}

	return Attribute->GetCurrentHealth();
}

bool AURGWeapon::ReportHit(const AActor* InTarget, const float InHealthBefore, const FVector& InLocation, const bool bInCrit) const
{
	// 맞기 전에 이미 죽었거나 캐릭터가 아니면 알릴 것이 없다
	if (InHealthBefore < 0.0f)
	{
		return false;
	}

	// 적은 사망 처리에서 바로 Destroy되지만, 실제 해제는 다음 GC라 같은 프레임 안에서는 컴포넌트를 읽을 수 있다
	const AURGCharacter* TargetCharacter = Cast<AURGCharacter>(InTarget);
	const UURGAttributeComponent* Attribute = TargetCharacter ? TargetCharacter->GetAttributeComponent() : nullptr;
	if (Attribute == nullptr)
	{
		return false;
	}

	const float DealtDamage = InHealthBefore - Attribute->GetCurrentHealth();
	if (DealtDamage <= 0.0f)
	{
		return false;
	}

	if (CombatComponent)
	{
		CombatComponent->NotifyHitDealt(InLocation, DealtDamage, bInCrit, !Attribute->IsAlive());
	}

	return true;
}

APlayerController* AURGWeapon::GetOwnerPlayerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
}

void AURGWeapon::PlayOwnerCameraShake(const float InScale) const
{
	if (CameraShakeClass == nullptr || InScale <= 0.0f)
	{
		return;
	}

	if (APlayerController* PlayerController = GetOwnerPlayerController())
	{
		PlayerController->ClientStartCameraShake(CameraShakeClass, InScale);
	}
}


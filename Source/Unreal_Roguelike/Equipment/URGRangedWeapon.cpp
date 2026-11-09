#include "Equipment/URGRangedWeapon.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Equipment/URGBullet.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "URGDefine.h"

namespace
{
	/** 발사음 피치 흔들림 폭 (±비율). 연사해도 같은 소리가 기계적으로 반복되지 않게 한다 */
	constexpr float FireSoundPitchJitter = 0.08f;
}

AURGRangedWeapon::AURGRangedWeapon()
{
	// 기본 코스메틱 총알 (BP에서 교체 가능)
	BulletClass = AURGBullet::StaticClass();
}

void AURGRangedWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	CurrentAmmo = MagazineSize;
}

void AURGRangedWeapon::Attack(const FGameplayTag InAttackType)
{
	if (!CanAttack())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (GetOwner() == nullptr || World == nullptr)
	{
		return;
	}

	Super::Attack(InAttackType);

	--CurrentAmmo;
	OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize);

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetMuzzleLocation(), 1.0f, FMath::FRandRange(1.0f - FireSoundPitchJitter, 1.0f + FireSoundPitchJitter));
	}

	// 조준 방향은 소유자의 시점(카메라) 기준으로 계산한다.
	FVector TraceStart;
	FRotator ViewRotation;
	GetOwnerViewPoint(TraceStart, ViewRotation);

	const FVector AimDirection = ViewRotation.Vector();

	// 머즐 파티클은 이번 공격에 쓰이는 머즐마다 한 번씩 (탄이 머즐 수보다 많아도 중복 스폰하지 않는다)
	if (MuzzleFX)
	{
		const int32 MuzzleCount = FMath::Min(ShotCount, FMath::Max(MuzzleSocketNames.Num(), 1));
		for (int32 MuzzleIndex = 0; MuzzleIndex < MuzzleCount; ++MuzzleIndex)
		{
			UGameplayStatics::SpawnEmitterAtLocation(World, MuzzleFX, GetMuzzleLocation(MuzzleIndex), AimDirection.Rotation());
		}
	}

	// 탄 수만큼 트레이스. 퍼짐이 있으면 조준 방향을 중심으로 한 원뿔 안에서 무작위 방향. 머즐은 순환
	const float SpreadHalfAngleRad = FMath::DegreesToRadians(SpreadAngle);
	for (int32 ShotIndex = 0; ShotIndex < ShotCount; ++ShotIndex)
	{
		const FVector ShotDirection = SpreadHalfAngleRad > 0.0f ? FMath::VRandCone(AimDirection, SpreadHalfAngleRad) : AimDirection;
		FireShot(TraceStart, ShotDirection, GetMuzzleLocation(ShotIndex));
	}

	// 조준 방향을 다 쓴 뒤에 반동을 줘야 이번 탄은 조준한 곳으로 나간다
	ApplyFireFeedback();
}

void AURGRangedWeapon::ApplyFireFeedback() const
{
	APlayerController* PlayerController = GetOwnerPlayerController();
	if (PlayerController == nullptr)
	{
		return;
	}

	PlayOwnerCameraShake(FireShakeScale);

	// 피치 상·하한은 카메라 매니저가 다음 회전 처리에서 잘라 준다
	if (RecoilPitch > 0.0f)
	{
		FRotator ControlRotation = PlayerController->GetControlRotation();
		ControlRotation.Pitch += RecoilPitch;
		PlayerController->SetControlRotation(ControlRotation);
	}
}

void AURGRangedWeapon::FireShot(const FVector& InTraceStart, const FVector& InDirection, const FVector& InMuzzleLocation)
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (OwnerActor == nullptr || World == nullptr)
	{
		return;
	}

	const FVector TraceEnd = InTraceStart + InDirection * FireRange;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponFire), true);
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(OwnerActor);

	const bool bHit = World->LineTraceSingleByChannel(Hit, InTraceStart, TraceEnd, ECC_WeaponTrace, Params);

	const FVector BulletEnd = bHit ? Hit.ImpactPoint : TraceEnd;
	const FRotator BulletRotation = (BulletEnd - InMuzzleLocation).Rotation();

	// 데미지는 발사 시점 값으로 고정 (총알 비행 중 공격 타입·부품 상태가 바뀌어도 이 탄에는 영향 없음)
	bool bShotCrit = false;
	const float ShotDamage = RollAttackDamage(bShotCrit);

	// 코스메틱 총알(트레이서) 발사 — 착탄 연출과 데미지 모두 총알이 명중 지점에 도착한 시점에 처리
	if (BulletClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = Cast<APawn>(OwnerActor);

		if (AURGBullet* Bullet = World->SpawnActor<AURGBullet>(BulletClass, InMuzzleLocation, BulletRotation, SpawnParams))
		{
			// 빗나간 경우 착탄 방향은 진행 반대 방향으로 둔다
			const FVector ImpactNormal = bHit ? Hit.ImpactNormal : -BulletRotation.Vector();

			// 무기가 먼저 제거되면(교체·소유자 사망) 도착 알림은 무시된다
			if (bHit)
			{
				Bullet->OnArrived.BindWeakLambda(this, [this, Hit, ShotDamage, bShotCrit]()
				{
					ApplyShotDamage(Hit, ShotDamage, bShotCrit);
				});
			}

			Bullet->Launch(InMuzzleLocation, BulletEnd, bHit, ImpactNormal);
			return;
		}
	}

	// 총알을 스폰하지 못한 경우에만 즉시 적용
	if (bHit)
	{
		ApplyShotDamage(Hit, ShotDamage, bShotCrit);
	}
}

void AURGRangedWeapon::ApplyShotDamage(const FHitResult& InHit, const float InDamage, const bool bInCrit)
{
	if (ExplosionRadius <= 0.0f)
	{
		if (InHit.GetActor())
		{
			ApplyDamageToHit(InHit, (InHit.TraceEnd - InHit.TraceStart).GetSafeNormal(), InDamage, bInCrit);
		}
		return;
	}

	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (OwnerActor == nullptr || World == nullptr)
	{
		return;
	}

	// 범위 데미지는 누가 얼마나 맞았는지 돌려주지 않으므로, 반경 안 캐릭터의 체력을 먼저 기록해 두고 뒤에 비교한다
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(URGExplosionOverlap), false, OwnerActor);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, InHit.ImpactPoint, FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllDynamicObjects),
		FCollisionShape::MakeSphere(ExplosionRadius), QueryParams);

	// 한 캐릭터가 캡슐·메시 등 여러 컴포넌트로 잡히므로 액터 단위로 모은다
	TMap<AActor*, float> HealthBefore;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (Candidate == nullptr || HealthBefore.Contains(Candidate))
		{
			continue;
		}

		const float Health = GetAliveHealth(Candidate);
		if (Health >= 0.0f)
		{
			HealthBefore.Add(Candidate, Health);
		}
	}

	// 범위 데미지: 착탄 지점 기준, 거리에 따라 감쇠. 소유자는 제외
	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(OwnerActor);
	IgnoredActors.Add(this);

	UGameplayStatics::ApplyRadialDamage(
		this,
		InDamage,
		InHit.ImpactPoint,
		ExplosionRadius,
		nullptr,
		IgnoredActors,
		this,
		OwnerActor->GetInstigatorController(),
		false,
		ECC_WeaponTrace);

	// 같은 프레임이라 이번 폭발에 죽어 Destroy된 적도 아직 메모리에 남아 있다
	for (const TPair<AActor*, float>& Pair : HealthBefore)
	{
		ReportHit(Pair.Key, Pair.Value, Pair.Key->GetActorLocation(), bInCrit);
	}
}

bool AURGRangedWeapon::CanAttack() const
{
	return HasAmmo();
}

bool AURGRangedWeapon::CanReload() const
{
	return CurrentAmmo < MagazineSize;
}

void AURGRangedWeapon::Reload()
{
	CurrentAmmo = MagazineSize;
	OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize);
}

void AURGRangedWeapon::RestoreAmmo(const int32 InAmmo)
{
	CurrentAmmo = FMath::Clamp(InAmmo, 0, MagazineSize);
	OnAmmoChanged.Broadcast(CurrentAmmo, MagazineSize);
}

bool AURGRangedWeapon::HasAmmo() const
{
	return CurrentAmmo > 0;
}

UAnimMontage* AURGRangedWeapon::GetReloadMontage() const
{
	return ReloadMontage;
}

FVector AURGRangedWeapon::GetMuzzleLocation(const int32 InShotIndex) const
{
	if (MuzzleSocketNames.IsEmpty())
	{
		return GetActorLocation();
	}

	return GetSocketLocationOrOwner(MuzzleSocketNames[InShotIndex % MuzzleSocketNames.Num()]);
}


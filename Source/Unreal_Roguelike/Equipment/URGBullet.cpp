#include "Equipment/URGBullet.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"

namespace
{
	/** 착탄음 피치 흔들림 폭 (±비율). 미사일 다발처럼 여러 발이 동시에 터져도 한 소리로 뭉개지지 않게 한다 */
	constexpr float ImpactSoundPitchJitter = 0.08f;
}

AURGBullet::AURGBullet()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BulletMesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	// Launch()가 불리지 않아도 자동 제거 (Launch가 비행 시간에 맞춰 다시 설정한다)
	InitialLifeSpan = 2.0f;
}

void AURGBullet::BeginPlay()
{
	Super::BeginPlay();

	if (TrailFX && Mesh)
	{
		UGameplayStatics::SpawnEmitterAttached(TrailFX, Mesh, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, FVector(TrailScale));
	}
}

void AURGBullet::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 수명 만료로 도착 전에 제거되는 경우에도 데미지는 적용되어야 한다 (월드 종료 시에는 보내지 않는다)
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		NotifyArrived();
	}

	Super::EndPlay(EndPlayReason);
}

void AURGBullet::Launch(const FVector& InStart, const FVector& InTarget, const bool bInHit, const FVector& InImpactNormal)
{
	SetActorLocation(InStart);
	StartLocation = InStart;
	TargetLocation = InTarget;
	ImpactNormal = InImpactNormal;
	bHitTarget = bInHit;
	bIsLaunched = true;

	// 비행 시간: 직선은 거리, 곡선은 꺾인 경로 길이 기준
	float PathLength = FVector::Dist(InStart, InTarget);

	if (ArcHeight > 0.0f)
	{
		// 제어점: 시작에서 목표 쪽으로 조금 간 지점 위 (정점이 ArcHeight가 되도록 2배) + 좌우 무작위 편차
		const FVector ToTarget = InTarget - InStart;
		const FVector Side = FVector::CrossProduct(ToTarget.GetSafeNormal2D(), FVector::UpVector);
		ArcControlPoint = InStart + ToTarget * 0.35f + FVector::UpVector * ArcHeight * 2.0f + Side * FMath::FRandRange(-ArcSideSpread, ArcSideSpread);

		PathLength = FVector::Dist(InStart, ArcControlPoint) + FVector::Dist(ArcControlPoint, InTarget);
		ArcDuration = FMath::Max(PathLength / FMath::Max(Speed, 1.0f), UE_KINDA_SMALL_NUMBER);
		ArcElapsed = 0.0f;
	}

	// 먼 목표(저격 사거리 끝)도 도착 전에 수명이 끝나지 않도록 비행 시간 + 여유로 수명을 잡는다
	SetLifeSpan(PathLength / FMath::Max(Speed, 1.0f) + 0.5f);
}

void AURGBullet::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsLaunched)
	{
		return;
	}

	if (ArcHeight > 0.0f)
	{
		TickArc(DeltaSeconds);
	}
	else
	{
		TickStraight(DeltaSeconds);
	}
}

void AURGBullet::TickStraight(const float DeltaSeconds)
{
	const FVector Current = GetActorLocation();
	const FVector ToTarget = TargetLocation - Current;
	const float Distance = ToTarget.Size();
	const float Step = Speed * DeltaSeconds;

	if (Distance <= Step || Distance < 1.0f)
	{
		Arrive();
		return;
	}

	SetActorLocation(Current + ToTarget / Distance * Step);
}

void AURGBullet::TickArc(const float DeltaSeconds)
{
	// 2차 베지어: 시작 → 제어점 위를 지나 → 목표
	ArcElapsed += DeltaSeconds;
	const float Alpha = ArcElapsed / ArcDuration;
	if (Alpha >= 1.0f)
	{
		SetActorLocation(TargetLocation);
		Arrive();
		return;
	}

	const float Inv = 1.0f - Alpha;
	const FVector Next = Inv * Inv * StartLocation + 2.0f * Inv * Alpha * ArcControlPoint + Alpha * Alpha * TargetLocation;
	const FVector Previous = GetActorLocation();
	SetActorLocationAndRotation(Next, (Next - Previous).GetSafeNormal().Rotation());
}

void AURGBullet::Arrive()
{
	PlayImpactEffects(TargetLocation, ImpactNormal, bHitTarget);
	NotifyArrived();
	Destroy();
}

USoundBase* AURGBullet::GetImpactSound() const
{
	return ImpactSound;
}

void AURGBullet::PlayImpactEffects(const FVector& Location, const FVector& Normal, bool bHit)
{
	// 착탄 연출은 도착 시점에 재생해 트레이서와 타이밍을 맞춘다
	UParticleSystem* ArrivalFX = bHit ? ImpactFX : MissFX;
	if (ArrivalFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, ArrivalFX, Location, Normal.Rotation(), FVector(ImpactScale));
	}

	if (bHit && ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Location, 1.0f, FMath::FRandRange(1.0f - ImpactSoundPitchJitter, 1.0f + ImpactSoundPitchJitter));
	}

}

void AURGBullet::NotifyArrived()
{
	if (bArrivedNotified)
	{
		return;
	}

	bArrivedNotified = true;
	OnArrived.ExecuteIfBound();
}


#include "Equipment/URGMeleeWeapon.h"
#include "URGDefine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "TimerManager.h"

namespace
{
	/** 휘두름·타격음 피치 흔들림 폭 (±비율). 같은 소리가 연달아 나도 기계적으로 반복되지 않게 한다 */
	constexpr float SoundPitchJitter = 0.08f;
}

AURGMeleeWeapon::AURGMeleeWeapon()
{
	// 판정 구간(BeginTrace~EndTrace)에만 켠다. 캐릭터 포즈 계산이 끝난 뒤의 소켓 위치를 읽도록 프레임 마지막 그룹에서 돈다
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void AURGMeleeWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateTrace();
}

void AURGMeleeWeapon::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitStopTimerHandle);
	}
	EndHitStop();

	Super::EndPlay(EndPlayReason);
}

void AURGMeleeWeapon::Attack()
{
	Super::Attack();

	if (SwingSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SwingSound, GetTraceLocation(), 1.0f, FMath::FRandRange(1.0f - SoundPitchJitter, 1.0f + SoundPitchJitter));
	}
}

void AURGMeleeWeapon::BeginTrace()
{
	HitActors.Reset();
	LastTraceLocation = GetTraceLocation();
	bIsTracing = true;
	SetActorTickEnabled(true);
}

void AURGMeleeWeapon::UpdateTrace()
{
	if (!bIsTracing)
	{
		return;
	}

	UWorld* World = GetWorld();
	AActor* OwnerActor = GetOwner();
	if (World == nullptr || OwnerActor == nullptr)
	{
		return;
	}

	// 큰 기체의 손은 작은 적의 머리 위로 지나가므로, 세로 캡슐을 윗반구가 소켓에 걸리게 아래로 매달아 손부터 그 아래까지 훑는다
	const float HalfHeight = FMath::Max(TraceHalfHeight, TraceRadius);
	const FVector HangOffset(0.0f, 0.0f, TraceRadius - HalfHeight);

	const FVector Start = LastTraceLocation + HangOffset;
	const FVector End = GetTraceLocation() + HangOffset;
	LastTraceLocation = End - HangOffset;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MeleeTrace), false);
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(OwnerActor);

	// 회전 없음 = 캡슐 축이 월드 Z
	TArray<FHitResult> Hits;
	World->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_WeaponTrace, FCollisionShape::MakeCapsule(TraceRadius, HalfHeight), Params);

	// 소켓이 제자리면 스윕 방향이 없으므로 캐릭터 전방을 데미지 방향으로 쓴다
	FVector SweepDirection = (End - Start).GetSafeNormal();
	if (SweepDirection.IsNearlyZero())
	{
		SweepDirection = OwnerActor->GetActorForwardVector();
	}

	for (const FHitResult& Hit : Hits)
	{
		ProcessHit(Hit, SweepDirection);
	}
}

void AURGMeleeWeapon::EndTrace()
{
	bIsTracing = false;
	HitActors.Reset();
	SetActorTickEnabled(false);
}

void AURGMeleeWeapon::ProcessHit(const FHitResult& InHit, const FVector& InSweepDirection)
{
	AActor* HitActor = InHit.GetActor();
	if (HitActor == nullptr || HitActors.Contains(HitActor))
	{
		return;
	}

	HitActors.Add(HitActor);

	if (HitFX != nullptr)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, HitFX, InHit.ImpactPoint, InHit.ImpactNormal.Rotation(), FVector(HitFXScale));
	}

	if (HitSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, InHit.ImpactPoint, 1.0f, FMath::FRandRange(1.0f - SoundPitchJitter, 1.0f + SoundPitchJitter));
	}

	// 벽·소품을 쳤을 때는 묵직한 연출을 주지 않는다
	if (ApplyDamageToHit(InHit, InSweepDirection))
	{
		PlayOwnerCameraShake(HitShakeScale);
		StartHitStop(HitActor);
	}
}

FVector AURGMeleeWeapon::GetTraceLocation() const
{
	return GetSocketLocationOrOwner(TraceSocketName);
}

void AURGMeleeWeapon::StartHitStop(AActor* InTarget)
{
	UWorld* World = GetWorld();
	AActor* OwnerActor = GetOwner();
	if (World == nullptr || OwnerActor == nullptr || HitStopDuration <= 0.0f)
	{
		return;
	}

	// 적이 근접 무기를 들어도 플레이어가 멈칫하지 않도록 플레이어가 휘두를 때만
	if (GetOwnerPlayerController() == nullptr)
	{
		return;
	}

	const auto Freeze = [this](AActor* InActor)
	{
		for (const TPair<TWeakObjectPtr<AActor>, float>& Pair : HitStopActors)
		{
			if (Pair.Key.Get() == InActor)
			{
				return;
			}
		}

		HitStopActors.Emplace(InActor, InActor->CustomTimeDilation);
		InActor->CustomTimeDilation *= HitStopTimeDilation;
	};

	Freeze(OwnerActor);

	// 이번 타격에 죽은 적은 곧 사라지므로 멈추지 않는다
	if (InTarget != nullptr && GetAliveHealth(InTarget) >= 0.0f)
	{
		Freeze(InTarget);
	}

	// 타이머는 전역 시간 배율(시간 감속 코어)만큼 느리게 흐르므로 곱해서 실제 초로 맞춘다
	const AWorldSettings* WorldSettings = World->GetWorldSettings();
	const float GlobalDilation = WorldSettings != nullptr ? WorldSettings->GetEffectiveTimeDilation() : 1.0f;
	World->GetTimerManager().SetTimer(HitStopTimerHandle, this, &AURGMeleeWeapon::EndHitStop, HitStopDuration * GlobalDilation, false);
}

void AURGMeleeWeapon::EndHitStop()
{
	for (const TPair<TWeakObjectPtr<AActor>, float>& Pair : HitStopActors)
	{
		if (AActor* Actor = Pair.Key.Get())
		{
			Actor->CustomTimeDilation = Pair.Value;
		}
	}

	HitStopActors.Reset();
}


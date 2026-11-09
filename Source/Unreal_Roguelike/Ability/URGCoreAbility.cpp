#include "Ability/URGCoreAbility.h"
#include "Characters/URGPlayerCharacter.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "TimerManager.h"

UWorld* UURGCoreAbility::GetWorld() const
{
	return OwnerCharacter.IsValid() ? OwnerCharacter->GetWorld() : nullptr;
}

void UURGCoreAbility::Initialize(AURGPlayerCharacter* InOwner)
{
	OwnerCharacter = InOwner;
}

bool UURGCoreAbility::TryActivate()
{
	if (!OwnerCharacter.IsValid() || bIsActive || !IsReady())
	{
		return false;
	}

	LastActivateTime = GetWorld()->GetTimeSeconds();
	bIsActive = true;
	Activate();

	// 공통 피드백: 파티클은 소유자에 붙이고, 문구는 캐릭터를 거쳐 HUD로
	if (ActivateEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(ActivateEffect, OwnerCharacter->GetRootComponent());
	}
	OwnerCharacter->NotifyCoreAbilityActivated(ActivateMessage);

	if (Duration > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(DurationTimer, this, &UURGCoreAbility::HandleDurationEnd, GetTimerDuration(), false);
	}
	else
	{
		// 즉발 능력은 발동과 동시에 끝난다
		bIsActive = false;
	}

	NotifyStateChanged();

	return true;
}

void UURGCoreAbility::Cancel()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DurationTimer);
	}

	if (bIsActive)
	{
		bIsActive = false;
		Deactivate();
		NotifyStateChanged();
	}
}

bool UURGCoreAbility::IsReady() const
{
	if (LastActivateTime < 0.0f)
	{
		return true;
	}

	const UWorld* World = GetWorld();
	return World && (World->GetTimeSeconds() - LastActivateTime) >= Cooldown * CooldownScale;
}

void UURGCoreAbility::HandleDurationEnd()
{
	bIsActive = false;
	Deactivate();
	NotifyStateChanged();
}

void UURGCoreAbility::NotifyStateChanged() const
{
	if (!OwnerCharacter.IsValid())
	{
		return;
	}

	// HUD가 남은 시간을 직접 계산하도록 끝나는 시각을 넘긴다. IsReady와 같은 월드 초 기준이라 시간 배율이 바뀌어도 어긋나지 않는다
	const float CooldownDuration = LastActivateTime < 0.0f ? 0.0f : Cooldown * CooldownScale;
	const float CooldownEndTime = LastActivateTime < 0.0f ? 0.0f : LastActivateTime + CooldownDuration;
	OwnerCharacter->NotifyCoreAbilityStateChanged(bIsActive, CooldownEndTime, CooldownDuration);
}

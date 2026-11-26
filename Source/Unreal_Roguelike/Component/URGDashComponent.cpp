#include "Component/URGDashComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

UURGDashComponent::UURGDashComponent()
{
	// 대시 입력 때만 동작하고 종료는 타이머로 받으므로 매 프레임 Tick은 필요하지 않다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UURGDashComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	check(OwnerCharacter);
}

void UURGDashComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelDash();

	Super::EndPlay(EndPlayReason);
}

bool UURGDashComponent::CanDash() const
{
	// 대시는 다리 부품이 제공한다. 빈 슬롯·파손·대시 없는 다리면 불가
	if (!bDashEnabled)
	{
		return false;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	return (Now - LastDashTime) >= DashCooldown * DashCooldownScale;
}

void UURGDashComponent::StartDash()
{
	LastDashTime = GetWorld()->GetTimeSeconds();

	// CanDash와 같은 기준(월드 초)으로 끝나는 시각을 넘겨, 위젯이 남은 시간을 직접 계산하게 한다
	const float CooldownDuration = DashCooldown * DashCooldownScale;
	OnDashCooldownChanged.Broadcast(LastDashTime + CooldownDuration, CooldownDuration);

	GetWorld()->GetTimerManager().SetTimer(DashTimer, this, &UURGDashComponent::HandleDashTimerExpired, DashDuration, false);

	// 수평 속도만 덮어쓴다 (공중 대시 시 낙하 속도는 유지)
	OwnerCharacter->LaunchCharacter(CalcDashDirection() * DashSpeed * DashSpeedScale, true, false);
}

void UURGDashComponent::CancelDash()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DashTimer);
	}
}

void UURGDashComponent::SetLegStats(bool bInDashEnabled, float InSpeedScale, float InCooldownScale)
{
	bDashEnabled = bInDashEnabled;
	DashSpeedScale = InSpeedScale;
	DashCooldownScale = InCooldownScale;

	// 쿨다운 도중 다리를 잃거나 대시 없는 다리로 바꾸면 이전 다리의 쿨다운 표시가 남지 않게 한다
	if (!bDashEnabled)
	{
		OnDashCooldownChanged.Broadcast(0.0f, 0.0f);
	}
}

FVector UURGDashComponent::CalcDashDirection() const
{
	FVector Direction = OwnerCharacter->GetPendingMovementInputVector();
	if (Direction.IsNearlyZero())
	{
		Direction = OwnerCharacter->GetLastMovementInputVector();
	}
	if (Direction.IsNearlyZero())
	{
		Direction = OwnerCharacter->GetActorForwardVector();
	}

	Direction.Z = 0.0f;
	return Direction.GetSafeNormal();
}

void UURGDashComponent::HandleDashTimerExpired()
{
	OnDashEnded.Broadcast();
}

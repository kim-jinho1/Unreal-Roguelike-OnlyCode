#include "Component/URGStateComponent.h"
#include "URGGameplayTags.h"

UURGStateComponent::UURGStateComponent()
{
	// 상태 변경은 전부 호출 기반이므로 매 프레임 Tick은 필요하지 않다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UURGStateComponent::BeginPlay()
{
	Super::BeginPlay();

	// 스폰 시 깨끗한 초기 상태 보장 (BP 아카이타입에 굳은 값이 있어도 강제 정상화)
	CurrentState = FGameplayTag::EmptyTag;
	bMovementInputEnabled = true;
}

void UURGStateComponent::SetCurrentState(const FGameplayTag& InNewState)
{
	// 사망은 마지막 상태다. 늦게 도착한 피격 종료·대시 종료 등이 되살리지 못하게 막는다.
	if (CurrentState == URGGameplayTags::Character_State_Death)
	{
		return;
	}

	CurrentState = InNewState;
}

void UURGStateComponent::ClearCurrentState()
{
	if (CurrentState == URGGameplayTags::Character_State_Death)
	{
		return;
	}

	CurrentState = FGameplayTag::EmptyTag;
}

bool UURGStateComponent::IsCurrentStateEqualToAny(const FGameplayTagContainer& TagsToCheck) const
{
	// 상태 미설정(빈 태그)이면 어떤 상태에도 해당하지 않음
	if (!CurrentState.IsValid())
	{
		return false;
	}

	return TagsToCheck.HasTagExact(CurrentState);
}

bool UURGStateComponent::IsCurrentStateEqualTo(const FGameplayTag& TagToCheck) const
{
	if (!CurrentState.IsValid())
	{
		return false;
	}

	return CurrentState.MatchesTagExact(TagToCheck);
}

void UURGStateComponent::SetMovementInputEnabled(bool bInEnabled)
{
	// 장부만 고친다. 잠금을 언제 풀지(타이머·노티파이)와 상태를 같이 지울지는 잠근 쪽이 정한다.
	bMovementInputEnabled = bInEnabled;
}

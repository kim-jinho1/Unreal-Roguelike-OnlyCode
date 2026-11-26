#include "Component/URGCoreAbilityComponent.h"
#include "Ability/URGCoreAbility.h"

UURGCoreAbilityComponent::UURGCoreAbilityComponent()
{
	// Q 입력과 코어 슬롯 변경 때만 동작하고 지속·쿨다운은 능력의 타이머와 월드 시각으로 처리하므로 Tick은 필요하지 않다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UURGCoreAbilityComponent::SetAbility(TSubclassOf<UURGCoreAbility> InAbilityClass, float InCooldownScale)
{
	// 클래스가 같으면 손상 단계만 바뀐 것이므로 인스턴스를 살려 쿨다운을 유지한다
	if (CoreAbility == nullptr || CoreAbility->GetClass() != InAbilityClass.Get())
	{
		if (CoreAbility != nullptr)
		{
			CoreAbility->Cancel();
			CoreAbility = nullptr;
		}

		if (InAbilityClass != nullptr)
		{
			CoreAbility = NewObject<UURGCoreAbility>(this, InAbilityClass);
			CoreAbility->Initialize(this);
		}

		// 새 능력은 쿨다운 없이 시작하고, 능력이 없어진 경우도 표시를 비워야 한다
		NotifyStateChanged(false, 0.0f, 0.0f);
	}

	if (CoreAbility != nullptr)
	{
		CoreAbility->SetCooldownScale(InCooldownScale);
	}
}

void UURGCoreAbilityComponent::TryActivate()
{
	if (CoreAbility != nullptr)
	{
		CoreAbility->TryActivate();
	}
}

void UURGCoreAbilityComponent::Cancel()
{
	if (CoreAbility != nullptr)
	{
		CoreAbility->Cancel();
	}
}

float UURGCoreAbilityComponent::GetIncomingDamageScale() const
{
	return CoreAbility != nullptr ? CoreAbility->GetIncomingDamageScale() : 1.0f;
}

void UURGCoreAbilityComponent::NotifyActivated(const FText& InMessage)
{
	OnCoreAbilityActivated.Broadcast(InMessage);
}

void UURGCoreAbilityComponent::NotifyStateChanged(bool bInActive, float InCooldownEndTime, float InCooldownDuration)
{
	OnCoreAbilityStateChanged.Broadcast(bInActive, InCooldownEndTime, InCooldownDuration);
}


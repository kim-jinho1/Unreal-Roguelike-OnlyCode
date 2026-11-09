#include "Component/URGResourceComponent.h"

UURGResourceComponent::UURGResourceComponent()
{
	// 자원 처리는 이벤트 기반이므로 Tick이 필요 없다
	PrimaryComponentTick.bCanEverTick = false;
}

void UURGResourceComponent::Add(const int32 InAmount)
{
	if (InAmount <= 0)
	{
		return;
	}

	Resource += InAmount;
	OnResourceChanged.Broadcast(Resource);
}

void UURGResourceComponent::SetResource(const int32 InAmount)
{
	Resource = FMath::Max(0, InAmount);
	OnResourceChanged.Broadcast(Resource);
}

bool UURGResourceComponent::TrySpend(const int32 InAmount)
{
	if (InAmount <= 0 || Resource < InAmount)
	{
		return false;
	}

	Resource -= InAmount;
	OnResourceChanged.Broadcast(Resource);
	return true;
}


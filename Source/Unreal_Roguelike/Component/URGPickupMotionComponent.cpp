#include "Component/URGPickupMotionComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"

UURGPickupMotionComponent::UURGPickupMotionComponent()
{
	// 회전·둥실거림 연출에 매 프레임 갱신이 필요하다
	PrimaryComponentTick.bCanEverTick = true;
}

void UURGPickupMotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (Owner != nullptr && RotationSpeed != 0.0f)
	{
		Owner->AddActorLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));
	}

	USceneComponent* Target = BobTarget.Get();
	if (Target == nullptr || BobHeight <= 0.0f || BobFrequency <= 0.0f)
	{
		return;
	}

	BobTime += DeltaTime;
	const float BobOffset = FMath::Sin(BobTime * BobFrequency * UE_TWO_PI) * BobHeight;
	Target->SetRelativeLocation(BobBaseLocation + FVector(0.0f, 0.0f, BobOffset));
}

void UURGPickupMotionComponent::SetBobTarget(USceneComponent* InTarget, const FVector& InBaseLocation)
{
	BobTarget = InTarget;
	BobBaseLocation = InBaseLocation;
}

void UURGPickupMotionComponent::SetMotionDefaults(float InRotationSpeed, float InBobHeight, float InBobFrequency)
{
	RotationSpeed = InRotationSpeed;
	BobHeight = InBobHeight;
	BobFrequency = InBobFrequency;
}


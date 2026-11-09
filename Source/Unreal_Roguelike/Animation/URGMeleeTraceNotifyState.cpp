#include "Animation/URGMeleeTraceNotifyState.h"
#include "Component/URGCombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Equipment/URGMeleeWeapon.h"

void UURGMeleeTraceNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AURGMeleeWeapon* MeleeWeapon = GetMeleeWeapon(MeshComp))
	{
		MeleeWeapon->BeginTrace();
	}
}

void UURGMeleeTraceNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (AURGMeleeWeapon* MeleeWeapon = GetMeleeWeapon(MeshComp))
	{
		MeleeWeapon->UpdateTrace();
	}
}

void UURGMeleeTraceNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AURGMeleeWeapon* MeleeWeapon = GetMeleeWeapon(MeshComp))
	{
		MeleeWeapon->EndTrace();
	}
}

FString UURGMeleeTraceNotifyState::GetNotifyName_Implementation() const
{
	return TEXT("URG Melee Trace");
}

AURGMeleeWeapon* UURGMeleeTraceNotifyState::GetMeleeWeapon(const USkeletalMeshComponent* InMeshComp) const
{
	const AActor* OwnerActor = InMeshComp ? InMeshComp->GetOwner() : nullptr;
	if (OwnerActor == nullptr)
	{
		return nullptr;
	}

	const UURGCombatComponent* CombatComponent = OwnerActor->FindComponentByClass<UURGCombatComponent>();
	if (CombatComponent == nullptr)
	{
		return nullptr;
	}

	return Cast<AURGMeleeWeapon>(CombatComponent->GetMainWeapon());
}


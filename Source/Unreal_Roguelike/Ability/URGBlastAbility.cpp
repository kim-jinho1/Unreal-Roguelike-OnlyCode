#include "Ability/URGBlastAbility.h"
#include "Characters/URGPlayerCharacter.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

void UURGBlastAbility::Activate()
{
	AURGPlayerCharacter* Owner = GetOwnerCharacter();
	if (Owner == nullptr)
	{
		return;
	}

	// 소유자는 제외하고 반경 안 전부에 같은 피해
	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(Owner);

	UGameplayStatics::ApplyRadialDamage(
		Owner,
		Damage,
		Owner->GetActorLocation(),
		Radius,
		DamageTypeClass ? *DamageTypeClass : UDamageType::StaticClass(),
		IgnoreActors,
		Owner,
		Owner->GetController(),
		/*bDoFullDamage*/ true);
}

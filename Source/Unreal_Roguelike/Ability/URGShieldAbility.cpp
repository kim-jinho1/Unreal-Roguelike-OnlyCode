#include "Ability/URGShieldAbility.h"
#include "Characters/URGPlayerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

UURGShieldAbility::UURGShieldAbility()
{
	Duration = 3.0f;
}

void UURGShieldAbility::Activate()
{
	AURGPlayerCharacter* Owner = GetOwnerCharacter();
	if (Owner == nullptr)
	{
		return;
	}

	Owner->SetIncomingDamageScale(DamageScale);

	if (ShieldMesh == nullptr)
	{
		return;
	}

	// 구체는 발동할 때 만들고 끝나면 없앤다 (부품 교체로 능력이 버려져도 캐릭터에 남지 않게)
	ShieldComponent = NewObject<UStaticMeshComponent>(Owner);
	ShieldComponent->SetStaticMesh(ShieldMesh);
	if (ShieldMaterial)
	{
		ShieldComponent->SetMaterial(0, ShieldMaterial);
	}
	ShieldComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShieldComponent->SetCastShadow(false);
	ShieldComponent->SetRelativeScale3D(FVector(ShieldScale));
	ShieldComponent->SetupAttachment(Owner->GetRootComponent());
	ShieldComponent->RegisterComponent();
}

void UURGShieldAbility::Deactivate()
{
	if (AURGPlayerCharacter* Owner = GetOwnerCharacter())
	{
		Owner->SetIncomingDamageScale(1.0f);
	}

	if (ShieldComponent)
	{
		ShieldComponent->DestroyComponent();
		ShieldComponent = nullptr;
	}
}

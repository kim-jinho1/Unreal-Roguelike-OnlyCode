#include "Equipment/URGEquipment.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"

AURGEquipment::AURGEquipment()
{
	PrimaryActorTick.bCanEverTick = false;

	// 소켓에 부착하려면 루트가 필요하다. 보이는 외형은 캐릭터 메시에 있으므로 위치만 갖는 빈 컴포넌트로 둔다
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AURGEquipment::EquipItem()
{
}

void AURGEquipment::UnequipItem()
{
}

void AURGEquipment::AttachToOwner(FName InSocketName)
{
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	USkeletalMeshComponent* CharacterMesh = OwnerCharacter->GetMesh();
	if (CharacterMesh == nullptr)
	{
		return;
	}

	// 위치/회전은 소켓에 스냅, 스케일은 장비 자체 상대 스케일을 유지(소켓 스케일에 덮이지 않게)
	const FAttachmentTransformRules Rules
	(
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::KeepRelative,
		true
	);

	AttachToComponent(CharacterMesh, Rules, InSocketName);
}

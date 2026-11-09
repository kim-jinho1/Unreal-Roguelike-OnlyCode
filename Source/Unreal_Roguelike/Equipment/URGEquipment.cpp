#include "Equipment/URGEquipment.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/Character.h"

AURGEquipment::AURGEquipment()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EquipmentMesh"));
	SetRootComponent(Mesh);

	Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

void AURGEquipment::BeginPlay()
{
	Super::BeginPlay();

	// 스폰 직후에도 메시가 확실히 적용되도록 한 번 더 반영
	if (Mesh && MeshAsset)
	{
		Mesh->SetStaticMesh(MeshAsset);
	}
}

void AURGEquipment::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 에디터 프리뷰 및 스폰 시 MeshAsset을 실제 메시 컴포넌트에 적용
	if (Mesh && MeshAsset)
	{
		Mesh->SetStaticMesh(MeshAsset);
	}
}

void AURGEquipment::EquipItem()
{
}

void AURGEquipment::UnequipItem()
{
}

void AURGEquipment::AttachToOwner(const FName InSocketName)
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

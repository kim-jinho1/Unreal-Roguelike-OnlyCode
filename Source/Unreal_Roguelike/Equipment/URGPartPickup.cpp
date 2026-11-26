#include "Equipment/URGPartPickup.h"
#include "Component/URGPartComponent.h"
#include "Component/URGPickupMotionComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Equipment/URGPartDataAsset.h"
#include "URGDefine.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** 부품 픽업 제자리 회전 속도 (도/초) */
	constexpr float PartPickupRotationSpeed = 60.0f;

	/** 부품 픽업 둥실거림 폭 (cm) */
	constexpr float PartPickupBobHeight = 8.0f;

	/** 부품 픽업 둥실거림 속도 (초당 왕복 횟수) */
	constexpr float PartPickupBobFrequency = 0.5f;
}

AURGPartPickup::AURGPartPickup()
{
	// 회전·둥실거림은 MotionComponent가 Tick하므로 액터 자체는 매 프레임 갱신할 일이 없다
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	Mesh->SetupAttachment(Root);

	// 상호작용 트레이스에만 걸리고 이동·사격·카메라는 통과한다
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_InteractTrace, ECR_Block);

	MotionComponent = CreateDefaultSubobject<UURGPickupMotionComponent>(TEXT("Motion"));
	MotionComponent->SetMotionDefaults(PartPickupRotationSpeed, PartPickupBobHeight, PartPickupBobFrequency);
}

void AURGPartPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyPartMesh();
}

void AURGPartPickup::SetPart(UURGPartDataAsset* InPart)
{
	Part = InPart;
	ApplyPartMesh();
}

void AURGPartPickup::Interact_Implementation(AActor* InInstigator)
{
	UURGPartComponent* PartComponent = InInstigator != nullptr ? InInstigator->FindComponentByClass<UURGPartComponent>() : nullptr;
	if (Part == nullptr || PartComponent == nullptr)
	{
		return;
	}

	// 슬롯에 있던 부품은 이 자리에 픽업으로 떨어뜨린다 (교환)
	UURGPartDataAsset* PreviousPart = PartComponent->GetSlotState(Part->GetSlot()).Part;

	PartComponent->EquipPart(Part);

	if (PreviousPart != nullptr)
	{
		if (UWorld* World = GetWorld())
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			if (AURGPartPickup* DroppedPickup = World->SpawnActor<AURGPartPickup>(GetClass(), GetActorTransform(), SpawnParams))
			{
				DroppedPickup->SetPart(PreviousPart);
			}
		}
	}

	Destroy();
}

FText AURGPartPickup::GetInteractText_Implementation() const
{
	return Part != nullptr ? FText::Format(InteractTextFormat, Part->GetDisplayName()) : FText::GetEmpty();
}

bool AURGPartPickup::CanInteract_Implementation(AActor* InInstigator) const
{
	return Part != nullptr && InInstigator != nullptr && InInstigator->FindComponentByClass<UURGPartComponent>() != nullptr;
}

void AURGPartPickup::ApplyPartMesh()
{
	if (Mesh == nullptr)
	{
		return;
	}

	UStaticMesh* PartMesh = Part != nullptr ? Part->GetPickupMesh() : nullptr;
	UStaticMesh* AppliedMesh = PartMesh != nullptr ? PartMesh : DefaultMesh.Get();
	Mesh->SetStaticMesh(AppliedMesh);

	if (AppliedMesh == nullptr)
	{
		return;
	}

	// 메시마다 원본 크기가 수 cm ~ 수 km로 제각각이라, 가장 긴 변이 TargetSize가 되게 균일 스케일을 구한다.
	// 픽업 클래스는 그대로 두고 메시만 바꿔 떨어뜨리는 경우가 있어서 BP에 스케일을 박지 않고 여기서 매번 계산한다.
	const FBoxSphereBounds MeshBounds = AppliedMesh->GetBounds();
	const float LongestSide = MeshBounds.BoxExtent.GetMax() * 2.0f;
	const float Scale = LongestSide > UE_KINDA_SMALL_NUMBER ? TargetSize / LongestSide : 1.0f;

	// 메시 원점이 바운즈 중심과 다르면 액터가 돌 때 메시가 큰 원을 그린다.
	// 기울기·스케일을 적용한 중심만큼 반대로 옮겨 바운즈 중심이 회전축(액터 원점) 위에 오게 한다.
	const FVector ScaledCenter = PickupTilt.RotateVector(MeshBounds.Origin * Scale);
	const FVector MeshBaseLocation = FVector(0.0f, 0.0f, HoverHeight) - ScaledCenter;

	Mesh->SetRelativeScale3D(FVector(Scale));
	Mesh->SetRelativeRotation(PickupTilt);
	Mesh->SetRelativeLocation(MeshBaseLocation);

	// 둥실거림은 이 기준 위치에 높이만 더한다 (메시가 바뀌면 기준도 바뀌므로 매번 다시 넘긴다)
	if (MotionComponent != nullptr)
	{
		MotionComponent->SetBobTarget(Mesh, MeshBaseLocation);
	}

	ApplySlotColor();
}

void AURGPartPickup::ApplySlotColor()
{
	if (PickupMaterial == nullptr || Part == nullptr)
	{
		return;
	}

	const FLinearColor* SlotColor = SlotColors.Find(Part->GetSlot());
	if (SlotColor == nullptr)
	{
		return;
	}

	static const FName ColorParameterName(TEXT("Color"));

	// 슬롯마다 색만 다르므로 머티리얼 애셋은 하나만 두고 픽업마다 다이내믹 인스턴스로 파라미터를 바꾼다.
	// 이미 같은 부모의 다이내믹 인스턴스가 붙어 있으면 엔진이 새로 만들지 않고 재사용한다 (교환으로 부품이 바뀔 때).
	for (int32 MaterialIndex = 0; MaterialIndex < Mesh->GetNumMaterials(); ++MaterialIndex)
	{
		if (UMaterialInstanceDynamic* DynamicMaterial = Mesh->CreateDynamicMaterialInstance(MaterialIndex, PickupMaterial))
		{
			DynamicMaterial->SetVectorParameterValue(ColorParameterName, *SlotColor);
		}
	}
}


#include "Equipment/URGPartPickup.h"
#include "Component/URGPartComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Equipment/URGPartDataAsset.h"
#include "URGDefine.h"
#include "Materials/MaterialInstanceDynamic.h"

AURGPartPickup::AURGPartPickup()
{
	// 회전·둥실거림 연출에 매 프레임 갱신이 필요하다
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	Mesh->SetupAttachment(Root);

	// 상호작용 트레이스에만 걸리고 이동·사격·카메라는 통과한다
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_InteractTrace, ECR_Block);
}

void AURGPartPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyPartMesh();
}

void AURGPartPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (RotationSpeed != 0.0f)
	{
		AddActorLocalRotation(FRotator(0.0f, RotationSpeed * DeltaSeconds, 0.0f));
	}

	// 액터는 세로축으로만 돌고, 둥실거림은 메시의 상대 위치로 준다 (액터 위치는 그대로 둬야 상호작용 거리가 흔들리지 않는다)
	if (BobHeight > 0.0f && BobFrequency > 0.0f)
	{
		BobTime += DeltaSeconds;
		const float BobOffset = FMath::Sin(BobTime * BobFrequency * UE_TWO_PI) * BobHeight;
		Mesh->SetRelativeLocation(MeshBaseLocation + FVector(0.0f, 0.0f, BobOffset));
	}
}

void AURGPartPickup::SetPart(UURGPartDataAsset* InPart)
{
	Part = InPart;
	ApplyPartMesh();
}

void AURGPartPickup::Interact_Implementation(AActor* InInstigator)
{
	UURGPartComponent* PartComponent = InInstigator ? InInstigator->FindComponentByClass<UURGPartComponent>() : nullptr;
	if (Part == nullptr || PartComponent == nullptr)
	{
		return;
	}

	// 슬롯에 있던 부품은 이 자리에 픽업으로 떨어뜨린다 (교환)
	UURGPartDataAsset* PreviousPart = PartComponent->GetSlotState(Part->GetSlot()).Part;

	PartComponent->EquipPart(Part);

	if (PreviousPart)
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
	return Part ? FText::Format(InteractTextFormat, Part->GetDisplayName()) : FText::GetEmpty();
}

bool AURGPartPickup::CanInteract_Implementation(AActor* InInstigator) const
{
	return Part != nullptr && InInstigator && InInstigator->FindComponentByClass<UURGPartComponent>() != nullptr;
}

void AURGPartPickup::ApplyPartMesh()
{
	if (Mesh == nullptr)
	{
		return;
	}

	UStaticMesh* PartMesh = Part ? Part->GetPickupMesh() : nullptr;
	UStaticMesh* AppliedMesh = PartMesh ? PartMesh : DefaultMesh.Get();
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
	MeshBaseLocation = FVector(0.0f, 0.0f, HoverHeight) - ScaledCenter;

	Mesh->SetRelativeScale3D(FVector(Scale));
	Mesh->SetRelativeRotation(PickupTilt);
	Mesh->SetRelativeLocation(MeshBaseLocation);

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


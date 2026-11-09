#include "Equipment/URGResourcePickup.h"
#include "Component/URGResourceComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"

AURGResourcePickup::AURGResourcePickup()
{
	// 회전·둥실거림 연출에 매 프레임 갱신이 필요하다
	PrimaryActorTick.bCanEverTick = true;

	CollectSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollectSphere"));
	SetRootComponent(CollectSphere);
	CollectSphere->InitSphereRadius(80.0f);

	// 폰과 겹침만 감지하고 이동·사격·카메라·상호작용 트레이스는 통과한다
	CollectSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollectSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollectSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollectSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollectSphere->SetGenerateOverlapEvents(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	Mesh->SetupAttachment(CollectSphere);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AURGResourcePickup::BeginPlay()
{
	Super::BeginPlay();

	MeshBaseLocation = Mesh->GetRelativeLocation();
	ApplyGlowColor();

	CollectSphere->OnComponentBeginOverlap.AddDynamic(this, &AURGResourcePickup::HandleCollectBeginOverlap);

	SnapToGround();

	// 근접 처치처럼 플레이어가 바로 옆에서 죽였으면 스폰 시점에 이미 겹쳐 있어 시작 겹침 이벤트가 다시 오지 않는다
	TArray<AActor*> OverlappingPawns;
	CollectSphere->GetOverlappingActors(OverlappingPawns, APawn::StaticClass());
	for (AActor* Pawn : OverlappingPawns)
	{
		if (TryCollect(Pawn))
		{
			return;
		}
	}
}

void AURGResourcePickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CollectSphere->OnComponentBeginOverlap.RemoveDynamic(this, &AURGResourcePickup::HandleCollectBeginOverlap);

	Super::EndPlay(EndPlayReason);
}

void AURGResourcePickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (RotationSpeed != 0.0f)
	{
		AddActorLocalRotation(FRotator(0.0f, RotationSpeed * DeltaSeconds, 0.0f));
	}

	// 수집 범위가 흔들리지 않도록 액터는 그대로 두고 메시만 위아래로 움직인다
	if (BobHeight > 0.0f && BobFrequency > 0.0f)
	{
		BobTime += DeltaSeconds;
		const float BobOffset = FMath::Sin(BobTime * BobFrequency * UE_TWO_PI) * BobHeight;
		Mesh->SetRelativeLocation(MeshBaseLocation + FVector(0.0f, 0.0f, BobOffset));
	}
}

void AURGResourcePickup::HandleCollectBeginOverlap(UPrimitiveComponent* InOverlappedComponent, AActor* InOtherActor, UPrimitiveComponent* InOtherComp, int32 InOtherBodyIndex, bool bInFromSweep, const FHitResult& InSweepResult)
{
	TryCollect(InOtherActor);
}

bool AURGResourcePickup::TryCollect(AActor* InActor)
{
	if (bCollected || InActor == nullptr)
	{
		return false;
	}

	// 적도 폰이라 겹침이 오므로 자원 컴포넌트가 있는 대상(플레이어)만 줍는다
	UURGResourceComponent* Resource = InActor->FindComponentByClass<UURGResourceComponent>();
	if (Resource == nullptr)
	{
		return false;
	}

	bCollected = true;
	Resource->Add(Amount);
	Destroy();
	return true;
}

void AURGResourcePickup::SnapToGround()
{
	UWorld* World = GetWorld();
	if (World == nullptr || GroundTraceDistance <= 0.0f)
	{
		return;
	}

	// 죽은 적의 메시·래그돌에 걸리지 않도록 고정 지형만 찾는다
	const FVector Start = GetActorLocation();
	const FVector End = Start - FVector(0.0f, 0.0f, GroundTraceDistance);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(URGResourcePickupGround), false, this);
	FHitResult Hit;
	if (World->LineTraceSingleByObjectType(Hit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		SetActorLocation(Hit.ImpactPoint + FVector(0.0f, 0.0f, HoverHeight));
	}
}

void AURGResourcePickup::ApplyGlowColor()
{
	if (PickupMaterial == nullptr)
	{
		return;
	}

	static const FName ColorParameterName(TEXT("Color"));

	for (int32 MaterialIndex = 0; MaterialIndex < Mesh->GetNumMaterials(); ++MaterialIndex)
	{
		if (UMaterialInstanceDynamic* DynamicMaterial = Mesh->CreateDynamicMaterialInstance(MaterialIndex, PickupMaterial))
		{
			DynamicMaterial->SetVectorParameterValue(ColorParameterName, GlowColor);
		}
	}
}

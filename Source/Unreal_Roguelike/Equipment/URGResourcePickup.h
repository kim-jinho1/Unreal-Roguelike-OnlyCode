#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "URGResourcePickup.generated.h"

/**
 * 적이 죽은 자리에 떨어지는 자원. 플레이어가 겹치면 ResourceComponent에 Amount를 더하고 사라진다.
 * 스폰되면 바로 아래 바닥으로 내려앉고, 제자리 회전 + 위아래로 둥실거린다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API AURGResourcePickup : public AActor
{
	GENERATED_BODY()

public:
	AURGResourcePickup();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaSeconds) override;

protected:
	/** 수집 범위에 들어온 액터에게 자원을 준다 */
	UFUNCTION()
	void HandleCollectBeginOverlap(UPrimitiveComponent* InOverlappedComponent, AActor* InOtherActor, UPrimitiveComponent* InOtherComp, int32 InOtherBodyIndex, bool bInFromSweep, const FHitResult& InSweepResult);

	/** 대상이 자원 컴포넌트를 가졌으면 자원을 더하고 픽업을 없앤다. 줬으면 true */
	bool TryCollect(AActor* InActor);

	/** 스폰 위치(적 몸 중심) 아래 바닥을 찾아 HoverHeight 위로 옮긴다. 바닥이 없으면 그 자리에 둔다 */
	void SnapToGround();

	/** 발광 색을 PickupMaterial의 다이내믹 인스턴스로 메시에 입힌다 */
	void ApplyGlowColor();

protected:
	/** 플레이어가 닿으면 수집되는 범위. 루트 */
	UPROPERTY(VisibleAnywhere, Category = "ResourcePickup|Collision")
	TObjectPtr<class USphereComponent> CollectSphere;

	/** 겉모습 메시 (BP에서 지정). 충돌 없음 */
	UPROPERTY(VisibleAnywhere, Category = "ResourcePickup|Mesh")
	TObjectPtr<class UStaticMeshComponent> Mesh;

	/** 주우면 더해지는 자원 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ResourcePickup|Resource", meta = (ClampMin = "1"))
	int32 Amount = 10;

	/** 바닥에서 액터 원점까지 띄우는 높이 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ResourcePickup|Visual", meta = (ClampMin = "0.0"))
	float HoverHeight = 30.0f;

	/** 스폰 위치에서 아래로 바닥을 찾는 최대 거리 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ResourcePickup|Visual", meta = (ClampMin = "0.0"))
	float GroundTraceDistance = 500.0f;

	/** 제자리 회전 속도 (도/초). 0이면 회전하지 않는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ResourcePickup|Visual")
	float RotationSpeed = 90.0f;

	/** 위아래로 둥실거리는 폭 (cm, 중심에서 한쪽). 0이면 움직이지 않는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ResourcePickup|Visual", meta = (ClampMin = "0.0"))
	float BobHeight = 6.0f;

	/** 둥실거림 속도 (초당 왕복 횟수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ResourcePickup|Visual", meta = (ClampMin = "0.0"))
	float BobFrequency = 0.6f;

	/** 발광 색을 입힐 머티리얼 (벡터 파라미터 Color 필요). 없으면 메시 원래 머티리얼 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ResourcePickup|Visual")
	TObjectPtr<class UMaterialInterface> PickupMaterial;

	/** 발광 색 (선형 색 공간). 부품 픽업 슬롯 색과 겹치지 않는 노란색 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ResourcePickup|Visual")
	FLinearColor GlowColor = FLinearColor(1.0f, 0.75f, 0.0f);

private:
	/** 메시의 기준 상대 위치 (BP 값). 둥실거림은 여기에 더한다 */
	FVector MeshBaseLocation = FVector::ZeroVector;

	/** 둥실거림 경과 시간 (초) */
	float BobTime = 0.0f;

	/** 같은 프레임에 겹침이 두 번 들어와도 자원을 한 번만 주기 위한 기록 */
	bool bCollected = false;

};

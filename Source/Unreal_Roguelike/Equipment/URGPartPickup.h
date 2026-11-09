#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/URGInteractable.h"
#include "URGDefine.h"
#include "URGPartPickup.generated.h"

/**
 * 바닥에 놓인 부품. 플레이어가 상호작용(E)하면 부품의 슬롯에 장착된다.
 * 그 슬롯에 이미 부품이 있었으면 그 부품이 같은 자리에 새 픽업으로 떨어진다(교환).
 * 메시는 부품 데이터의 PickupMesh, 없으면 DefaultMesh를 쓴다.
 * 메시마다 원본 크기·중심이 제각각이라 바운즈로 크기를 TargetSize에 맞추고 중심을 액터 원점에 맞춘 뒤,
 * 기울인 채로 제자리 회전 + 위아래로 둥실거리게 한다. 부품 슬롯별 색으로 발광시켜 슬롯을 구분한다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API AURGPartPickup : public AActor, public IURGInteractable
{
	GENERATED_BODY()

public:
	AURGPartPickup();

protected:
	/** 부품 데이터에 맞는 메시 적용 (에디터/스폰 시) */
	virtual void OnConstruction(const FTransform& Transform) override;

public:
	virtual void Tick(float DeltaSeconds) override;

public:
	FORCEINLINE class UURGPartDataAsset* GetPart() const { return Part; }

	/** 스폰 직후 부품 지정 (교환으로 떨어지는 픽업용). 메시도 갱신한다 */
	void SetPart(class UURGPartDataAsset* InPart);

	/** IURGInteractable */
	virtual void Interact_Implementation(AActor* InInstigator) override;
	virtual FText GetInteractText_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* InInstigator) const override;

protected:
	/** Part의 PickupMesh 또는 DefaultMesh를 Mesh 컴포넌트에 적용하고 크기·중심·기울기를 맞춘다 */
	void ApplyPartMesh();

	/** 부품 슬롯 색을 PickupMaterial의 다이내믹 인스턴스로 메시에 입힌다 */
	void ApplySlotColor();

protected:
	/** 회전 축이 되는 루트. 메시를 자식으로 두어야 메시 중심을 액터 원점에 맞출 수 있다 */
	UPROPERTY(VisibleAnywhere, Category = "PartPickup|Mesh")
	TObjectPtr<class USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "PartPickup|Mesh")
	TObjectPtr<class UStaticMeshComponent> Mesh;

	/** 이 픽업이 주는 부품 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PartPickup|Part")
	TObjectPtr<class UURGPartDataAsset> Part;

	/** 부품 데이터에 PickupMesh가 없을 때 쓰는 메시 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PartPickup|Mesh")
	TObjectPtr<class UStaticMesh> DefaultMesh;

	/** 제자리 회전 속도 (도/초). 0이면 회전하지 않는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PartPickup|Visual")
	float RotationSpeed = 60.0f;

	/** 메시의 가장 긴 변을 이 길이(cm)로 맞춘다. 원본 크기와 상관없이 픽업 크기가 같아진다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PartPickup|Visual", meta = (ClampMin = "1.0"))
	float TargetSize = 40.0f;

	/** 메시 기울기. 납작한 부품이 누운 채로 돌면 옆에서 선으로 보이므로 세워서 돌린다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PartPickup|Visual")
	FRotator PickupTilt = FRotator(60.0f, 0.0f, 0.0f);

	/** 액터 원점에서 메시 중심을 띄우는 높이 (cm). 바닥에 직접 놓는 픽업용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PartPickup|Visual")
	float HoverHeight = 0.0f;

	/** 위아래로 둥실거리는 폭 (cm, 중심에서 한쪽). 0이면 움직이지 않는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PartPickup|Visual", meta = (ClampMin = "0.0"))
	float BobHeight = 8.0f;

	/** 둥실거림 속도 (초당 왕복 횟수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PartPickup|Visual", meta = (ClampMin = "0.0"))
	float BobFrequency = 0.5f;

	/** 슬롯 색을 입힐 머티리얼 (벡터 파라미터 Color 필요). 없으면 메시 원래 머티리얼 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PartPickup|Visual")
	TObjectPtr<class UMaterialInterface> PickupMaterial;

	/** 슬롯별 발광 색 (선형 색 공간). 부품 종류는 상호작용 문구로, 슬롯은 색으로 구분한다 */
	// 선형 0.1이 화면에서는 약 35% 밝기라, 주색이 아닌 채널은 0에 가깝게 둬야 파스텔로 뜨지 않는다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PartPickup|Visual")
	TMap<EURGPartSlot, FLinearColor> SlotColors = {
		{ EURGPartSlot::Arm, FLinearColor(1.0f, 0.01f, 0.005f) },
		{ EURGPartSlot::Leg, FLinearColor(0.01f, 1.0f, 0.02f) },
		{ EURGPartSlot::Core, FLinearColor(0.005f, 0.05f, 1.0f) },
		{ EURGPartSlot::Head, FLinearColor(1.0f, 0.45f, 0.0f) },
	};

	/** 상호작용 안내 문구 포맷 ({0} = 부품 이름) */
	UPROPERTY(EditDefaultsOnly, Category = "PartPickup|Text")
	FText InteractTextFormat = NSLOCTEXT("URG.PartPickup", "InteractFormat", "{0} 장착");

private:
	/** ApplyPartMesh가 계산한 메시 기준 위치 (중심 보정 + HoverHeight). 둥실거림은 여기에 더한다 */
	FVector MeshBaseLocation = FVector::ZeroVector;

	/** 둥실거림 경과 시간 (초) */
	float BobTime = 0.0f;

};


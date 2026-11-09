#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "URGEquipment.generated.h"

/**
 * 장착 가능한 장비 액터의 베이스.
 * 스태틱 메시 하나를 가지며 소유 캐릭터의 메시 소켓에 부착된다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API AURGEquipment : public AActor
{
	GENERATED_BODY()

public:
	AURGEquipment();

protected:
	virtual void BeginPlay() override;

	/** MeshAsset을 Mesh 컴포넌트에 적용 (에디터/스폰 시) */
	virtual void OnConstruction(const FTransform& Transform) override;

public:
	FORCEINLINE UStaticMeshComponent* GetMesh() const { return Mesh; }
	FORCEINLINE UStaticMesh* GetMeshAsset() const { return MeshAsset; }
	FORCEINLINE UTexture2D* GetEquipmentIcon() const { return EquipmentIcon; }

public:
	/** 장착 처리. 파생 클래스가 소유자 컴포넌트 등록/부착을 수행한다 */
	virtual void EquipItem();

	/** 장착 해제 처리 */
	virtual void UnequipItem();

	/** 소유 캐릭터 메시의 소켓에 부착 (위치/회전 스냅, 스케일은 상대값 유지) */
	virtual void AttachToOwner(FName InSocketName);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Equipment|Mesh")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, Category = "Equipment|Mesh")
	TObjectPtr<UStaticMesh> MeshAsset;

	UPROPERTY(EditAnywhere, Category = "Equipment|Icon")
	TObjectPtr<UTexture2D> EquipmentIcon;

};

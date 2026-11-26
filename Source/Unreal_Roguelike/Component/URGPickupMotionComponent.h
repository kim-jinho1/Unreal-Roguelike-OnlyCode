#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "URGPickupMotionComponent.generated.h"

/**
 * 픽업 연출 컴포넌트: 소유 액터를 세로축으로 제자리 회전시키고, 지정한 메시만 위아래로 둥실거리게 한다.
 * 액터 위치는 건드리지 않으므로 상호작용 거리·수집 범위가 흔들리지 않는다.
 * 둥실거릴 메시와 기준 위치는 소유 액터가 SetBobTarget으로 넘긴다 (메시를 바꾸면 다시 부른다).
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_ROGUELIKE_API UURGPickupMotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UURGPickupMotionComponent();

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	/** 둥실거릴 컴포넌트와 그 기준 상대 위치. 둥실거림은 기준 위치에 높이만 더한다 */
	void SetBobTarget(class USceneComponent* InTarget, const FVector& InBaseLocation);

	/** 기본값 지정 (소유 액터 생성자에서 픽업 종류별로) */
	void SetMotionDefaults(float InRotationSpeed, float InBobHeight, float InBobFrequency);

protected:
	/** 제자리 회전 속도 (도/초). 0이면 회전하지 않는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PickupMotion|Rotation")
	float RotationSpeed = 60.0f;

	/** 위아래로 둥실거리는 폭 (cm, 중심에서 한쪽). 0이면 움직이지 않는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PickupMotion|Bob", meta = (ClampMin = "0.0"))
	float BobHeight = 8.0f;

	/** 둥실거림 속도 (초당 왕복 횟수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PickupMotion|Bob", meta = (ClampMin = "0.0"))
	float BobFrequency = 0.5f;

private:
	/** 둥실거릴 컴포넌트 (보통 소유 액터의 메시) */
	TWeakObjectPtr<class USceneComponent> BobTarget;

	/** 둥실거림 기준 상대 위치 */
	FVector BobBaseLocation = FVector::ZeroVector;

	/** 둥실거림 경과 시간 (초) */
	float BobTime = 0.0f;

};


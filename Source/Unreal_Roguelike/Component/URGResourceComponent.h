#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "URGResourceComponent.generated.h"

/** 보유 자원 변경 알림 (NewResource) */
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateOnResourceChanged, int32);

/**
 * 보유 자원 관리 컴포넌트 (기획서의 "보유 자원").
 * 획득/소비는 이벤트 기반이며, HUD는 델리게이트로만 값을 받는다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_ROGUELIKE_API UURGResourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UURGResourceComponent();

public:
	FORCEINLINE int32 GetResource() const { return Resource; }

	/** 자원 추가 (0 이하는 무시) */
	void Add(int32 InAmount);

	/** 자원 소비 시도. 잔액이 충분하면 차감하고 true 반환 */
	bool TrySpend(int32 InAmount);

	/** 보유 자원을 지정 값으로 덮어쓴다 (스테이지 이동 복원용) */
	void SetResource(int32 InAmount);

public:
	/** 자원이 변경될 때 호출 */
	FDelegateOnResourceChanged OnResourceChanged;

protected:
	/** 보유 자원 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resource|State")
	int32 Resource = 0;

};


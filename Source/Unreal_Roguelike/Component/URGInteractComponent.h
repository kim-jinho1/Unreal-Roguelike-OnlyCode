#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "URGInteractComponent.generated.h"

/** 바라보는 상호작용 대상 변경 알림 (새 대상, 없으면 nullptr) */
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateOnInteractFocusChanged, AActor*);

/**
 * 상호작용 컴포넌트: 카메라 시점 InteractTrace로 IURGInteractable 대상을 찾는다.
 * 짧은 간격으로 바라보는 대상(포커스)을 갱신해 바뀔 때만 알리고, 요청이 오면 그 대상과 상호작용한다.
 * 언제 상호작용할 수 있는지(사망 등)는 소유자가 정한다. 소유자는 APawn이어야 한다 (컨트롤러 시점을 쓴다).
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_ROGUELIKE_API UURGInteractComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UURGInteractComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** 지금 바라보는 상호작용 대상 (상호작용하면 이 액터가 받는다). 없으면 nullptr */
	FORCEINLINE AActor* GetInteractFocus() const { return InteractFocus.Get(); }

	/** 지금 바라보는 대상을 찾아 상호작용한다. 대상이 있었으면 true */
	bool TryInteract();

	/** 포커스 갱신을 멈추고 포커스를 비운다 (사망 시). 비교 UI 등이 남지 않게 nullptr를 알린다 */
	void StopFocus();

	/** 바라보는 상호작용 대상이 바뀔 때 호출 */
	FDelegateOnInteractFocusChanged OnInteractFocusChanged;

protected:
	/** 카메라 시점 InteractTrace로 지금 상호작용할 수 있는 대상을 찾는다. 없으면 nullptr. CanInteract에 소유자를 넘기므로 non-const */
	AActor* FindInteractTarget();

	/** 포커스 타이머 콜백: 바라보는 대상을 다시 찾고 바뀌었으면 알린다 */
	void UpdateInteractFocus();

	/** 포커스 대상 기록 + 바뀌었을 때만 OnInteractFocusChanged */
	void SetInteractFocus(AActor* InActor);

protected:
	/** 상호작용 탐색 거리 (소유자 기준, cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interact|Trace")
	float InteractRange = 300.0f;

	/** 바라보는 상호작용 대상을 다시 찾는 간격 (초). 매 프레임 대신 이 간격으로만 트레이스한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interact|Trace", meta = (ClampMin = "0.02"))
	float InteractFocusInterval = 0.1f;

private:
	/** 소유 폰 (BeginPlay에서 한 번 캐싱) */
	UPROPERTY()
	TObjectPtr<class APawn> OwnerPawn;

	/** 바라보는 상호작용 대상 갱신 타이머 */
	FTimerHandle InteractFocusTimer;

	/** 지금 바라보는 상호작용 대상 (픽업이 주워져 사라질 수 있으므로 약참조) */
	TWeakObjectPtr<AActor> InteractFocus;

};

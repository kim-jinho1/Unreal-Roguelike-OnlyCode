#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "URGGameplayTags.h"
#include "URGStateComponent.generated.h"

/**
 * 캐릭터의 현재 상태(GameplayTag 하나)와 이동 입력 잠금을 관리한다.
 * 대시·재장전·피격·사망 중 입력 차단 판정은 전부 이 컴포넌트를 거친다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_ROGUELIKE_API UURGStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UURGStateComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** 현재 상태 설정. 사망 상태는 덮어쓸 수 없다 */
	FORCEINLINE void SetCurrentState(const FGameplayTag NewState)
	{
		if (CurrentState == URGGameplayTags::Character_State_Death)
		{
			return;
		}

		CurrentState = NewState;
	}

	FORCEINLINE FGameplayTag GetCurrentState() const { return CurrentState; }

	/** 현재 상태 초기화 (사망 상태는 유지) */
	void ClearCurrentState();

	/** 현재 상태가 주어진 태그 중 하나와 일치하는지 확인 (OR) */
	bool IsCurrentStateEqualToAny(const FGameplayTagContainer& TagsToCheck) const;

	/** 현재 상태가 주어진 태그와 정확히 일치하는지 확인 */
	bool IsCurrentStateEqualTo(const FGameplayTag& TagToCheck) const;

public:
	FORCEINLINE bool MovementInputEnabled() const { return bMovementInputEnabled; }

	/** 이동 입력 토글 */
	void ToggleMovementInput(bool bEnabled, float Duration = 0.f);

	/** 이동 입력 해제 + 상태 초기화 (타이머 콜백) */
	void MovementInputEnableAction();

protected:
	/** 캐릭터의 현재 상태 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Runtime")
	FGameplayTag CurrentState;

	/** 이동키 입력 가능 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Runtime")
	bool bMovementInputEnabled = true;

	/** 이동 입력 잠금 해제 예약 타이머 핸들 */
	FTimerHandle MovementUnlockTimer;

};

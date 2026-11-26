#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "URGCoreAbilityComponent.generated.h"

/** 코어 능력 발동 알림 (HUD 문구용). 인자는 능력이 정한 메시지 */
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateOnCoreAbilityActivated, const FText&);

/** 코어 능력 상태 변경 알림 (bActive 지속 중 여부, CooldownEndTime 쿨다운이 끝나는 월드 초, CooldownDuration 전체 쿨다운 초). 쿨다운이 없으면 둘 다 0 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FDelegateOnCoreAbilityStateChanged, bool, float, float);

/**
 * 코어 능력 컴포넌트: 코어 부품이 주는 Q 능력 인스턴스를 보관하고 교체·발동·취소한다.
 * 능력의 발동·상태 변화는 이 컴포넌트의 델리게이트로 HUD에 알린다.
 * 어떤 능력을 쓸지(코어 슬롯 상태)와 지금 Q를 눌러도 되는지(사망 등)는 소유 캐릭터가 정한다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_ROGUELIKE_API UURGCoreAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UURGCoreAbilityComponent();

public:
	/**
	 * 코어 슬롯 반영: 능력 클래스가 바뀌면(교체·제거·파손) 진행 중인 효과를 끊고 새로 만든다.
	 * 클래스가 같으면(손상 단계만 바뀜) 인스턴스와 쿨다운을 유지하고 배율만 바꾼다. nullptr이면 능력 제거
	 */
	void SetAbility(TSubclassOf<class UURGCoreAbility> InAbilityClass, float InCooldownScale);

	/** 능력이 있으면 발동 시도 (쿨다운·지속 중 검사는 능력이 한다) */
	void TryActivate();

	/** 발동 중인 효과를 즉시 끊는다 (사망용) */
	void Cancel();

	/** 받는 피해 배율. 능력이 없으면 1 (실드 같은 능력이 발동 중에만 낮춘다) */
	float GetIncomingDamageScale() const;

	/** 능력이 발동됐음을 알린다 (능력이 호출) */
	void NotifyActivated(const FText& InMessage);

	/** 능력의 지속·쿨다운 상태가 바뀌었음을 알린다 (능력이 호출) */
	void NotifyStateChanged(bool bInActive, float InCooldownEndTime, float InCooldownDuration);

	/** 능력 발동 시 호출 */
	FDelegateOnCoreAbilityActivated OnCoreAbilityActivated;

	/** 능력 발동·종료·교체·쿨다운 배율 변경 시 호출 (HUD 쿨다운 표시용) */
	FDelegateOnCoreAbilityStateChanged OnCoreAbilityStateChanged;

private:
	/** 코어 부품이 제공하는 능력 인스턴스 (SetAbility에서 생성·교체). 없으면 Q 무시 */
	UPROPERTY()
	TObjectPtr<class UURGCoreAbility> CoreAbility;

};


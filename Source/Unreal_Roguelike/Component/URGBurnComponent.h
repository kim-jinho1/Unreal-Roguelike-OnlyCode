#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "URGBurnComponent.generated.h"

/**
 * 화상 지속 피해 컴포넌트 (열 시너지).
 * 공격자의 CombatComponent가 적중한 대상에 런타임으로 붙이고, 대상의 데미지 파이프라인(ApplyDamage)으로만 피해를 준다.
 * 적 클래스를 수정하지 않고 어떤 캐릭터에도 붙일 수 있다. 지속 시간이 끝나거나 대상이 죽으면 스스로 제거된다.
 */
UCLASS(ClassGroup = (Custom))
class UNREAL_ROGUELIKE_API UURGBurnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UURGBurnComponent();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** 화상 시작. 이미 타는 중이면 수치를 덮어쓰고 남은 횟수를 처음부터 다시 센다 */
	void StartBurn(AActor* InCauser, float InDamagePerTick, float InInterval, float InDuration);

private:
	/** 타이머 콜백: 한 번 피해를 주고 남은 횟수가 없거나 대상이 죽었으면 종료 */
	void HandleBurnTick();

	/** 타이머를 멈추고 컴포넌트를 제거한다 */
	void StopBurn();

private:
	/** 피해를 준 액터 (사망 연출·가해자 기록용). 먼저 사라질 수 있으므로 약참조 */
	TWeakObjectPtr<AActor> Causer;

	/** 한 번에 주는 피해 */
	float DamagePerTick = 0.0f;

	/** 남은 피해 횟수 */
	int32 RemainingTicks = 0;

	/** 피해 간격 타이머 */
	FTimerHandle BurnTimer;

};


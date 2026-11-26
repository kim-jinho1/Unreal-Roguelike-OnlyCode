#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "URGDefine.h"
#include "URGHitReactComponent.generated.h"

/** 피격 리액션 몽타주 종료 알림 (bInterrupted: 다른 몽타주·사망에 끊겼는지) */
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateOnHitReactEnded, bool);

/**
 * 피격·사망 연출 컴포넌트: 방향별 리액션 몽타주, 신음, 사망 비명, 래그돌 밀침.
 * 연출을 할지 말지(사망·대시 검사)와 상태 태그 변경은 소유 캐릭터가 정한다. 이 컴포넌트는 소리·몽타주·물리만 다룬다.
 * 소유자는 ACharacter여야 한다 (메시와 몽타주 재생을 쓴다).
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_ROGUELIKE_API UURGHitReactComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UURGHitReactComponent();

protected:
	virtual void BeginPlay() override;

public:
	/**
	 * 신음 재생 후 가해자 방향의 리액션 몽타주 재생.
	 * InDamageRatio는 최대 체력 대비 피해 비율 (큰 신음 판정용). 가해자를 모르면 정면 피격으로 본다.
	 * 몽타주를 틀었으면 true — 끝날 때 OnHitReactEnded가 온다.
	 */
	bool PlayHitReact(float InDamageRatio, const AActor* InCauser);

	/** 사망 비명 + 래그돌 전환 + 가해자 반대 방향 밀침 (가해자를 모르면 제자리에서 쓰러진다) */
	void PlayDeath(const AActor* InCauser);

	/** 리액션 몽타주가 끝나거나 끊겼을 때 호출 */
	FDelegateOnHitReactEnded OnHitReactEnded;

protected:
	/** 피해 비율에 맞는 신음 재생. 일반 신음은 PainSoundInterval 안에 다시 울리지 않는다 */
	void PlayPainSound(float InDamageRatio);

	/** 가해자 위치가 소유자 기준 전/후/좌/우 어느 쪽인지 계산 */
	EURGHitDirection CalcHitDirection(const FVector& InSourceLocation) const;

	/** 리액션 몽타주 종료 → OnHitReactEnded로 전달 */
	void HandleMontageEnded(class UAnimMontage* InMontage, bool bInterrupted);

protected:
	/** 피격 방향별 상체 리액션 몽타주 (UpperBody 슬롯). 없는 방향은 리액션 생략 */
	UPROPERTY(EditDefaultsOnly, Category = "HitReact|Animation")
	TMap<EURGHitDirection, TObjectPtr<class UAnimMontage>> HitReactMontages;

	/** 일반 피격 신음 */
	UPROPERTY(EditDefaultsOnly, Category = "HitReact|Sound")
	TObjectPtr<class USoundBase> PainSound;

	/** 큰 피격 신음 (한 번에 PainHeavyDamageRatio 이상 깎였을 때) */
	UPROPERTY(EditDefaultsOnly, Category = "HitReact|Sound")
	TObjectPtr<class USoundBase> PainHeavySound;

	/** 사망 비명 */
	UPROPERTY(EditDefaultsOnly, Category = "HitReact|Sound")
	TObjectPtr<class USoundBase> DeathSound;

	/** 큰 신음으로 바꾸는 피해 비율 (최대 체력 대비) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReact|Sound", meta = (ClampMin = "0", ClampMax = "1"))
	float PainHeavyDamageRatio = 0.15f;

	/** 일반 신음 최소 간격 (초). 여러 적에게 동시에 맞아도 신음이 겹치지 않게 한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReact|Sound", meta = (ClampMin = "0"))
	float PainSoundInterval = 0.4f;

	/** 사망 시 마지막 가해자 반대 방향으로 래그돌에 주는 속도 변화량 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReact|Death")
	float DeathImpulse = 600.0f;

	/** 사망 밀침 방향에 섞는 위쪽 성분 비율 (0 = 수평, 1 = 45°) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReact|Death", meta = (ClampMin = "0", ClampMax = "1"))
	float DeathImpulseUpRatio = 0.3f;

private:
	/** 소유 캐릭터 (BeginPlay에서 한 번 캐싱) */
	UPROPERTY()
	TObjectPtr<class ACharacter> OwnerCharacter;

	/** 마지막 신음 재생 시각 (PainSoundInterval 계산용) */
	float LastPainSoundTime = -1.0f;

};

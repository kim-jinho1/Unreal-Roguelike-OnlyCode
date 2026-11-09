#pragma once

#include "CoreMinimal.h"
#include "Equipment/URGWeapon.h"
#include "URGMeleeWeapon.generated.h"

/**
 * 근접 무기.
 * 공격 몽타주의 URGMeleeTrace 노티파이 구간 동안 판정 소켓의 이동 궤적을 구체로 스윕해
 * WeaponTrace 채널에 걸리는 액터에 한 번씩 데미지를 적용한다. 판정 시작/갱신/종료는 노티파이가 호출한다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API AURGMeleeWeapon : public AURGWeapon
{
	GENERATED_BODY()

public:
	AURGMeleeWeapon();

protected:
	/** 히트스톱 도중 무기가 제거되면(무기 교체·소유자 사망) 늦춰 둔 시간 배율을 되돌린다 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** 공격 타입을 기록하고 휘두름 소리를 재생한다. 판정은 노티파이 구간에서 수행 */
	virtual void Attack(FGameplayTag InAttackType) override;

public:
	/** 판정 시작 (노티파이 Begin): 명중 목록 초기화, 소켓 현재 위치 기록 */
	void BeginTrace();

	/** 판정 갱신 (노티파이 Tick): 지난 위치 → 현재 위치 구체 스윕 */
	void UpdateTrace();

	/** 판정 종료 (노티파이 End) */
	void EndTrace();

	FORCEINLINE bool IsTracing() const { return bIsTracing; }

protected:
	/** 스윕 결과 중 아직 맞지 않은 액터에 데미지·연출 적용 */
	void ProcessHit(const FHitResult& InHit, const FVector& InSweepDirection);

	/** 판정 소켓 월드 위치 */
	FVector GetTraceLocation() const;

	/** 적중 순간 소유 플레이어와 맞은 대상을 잠깐 거의 멈춘다. 이미 히트스톱 중이면 대상만 추가하고 시간을 다시 잰다 */
	void StartHitStop(AActor* InTarget);

	/** 히트스톱 종료: 늦춘 액터들의 시간 배율을 시작 전 값으로 되돌린다 */
	void EndHitStop();

protected:
	/** 판정 구체 반지름 (cm) */
	UPROPERTY(EditAnywhere, Category = "MeleeWeapon|Trace", meta = (ClampMin = "1"))
	float TraceRadius = 40.0f;

	/** 판정 소켓 이름 (무기 메시 또는 소유 캐릭터 메시) */
	UPROPERTY(EditAnywhere, Category = "MeleeWeapon|Trace")
	FName TraceSocketName = TEXT("hand_r");

	/** 휘두름 소리 (공격 시작 시) */
	UPROPERTY(EditAnywhere, Category = "MeleeWeapon|Sound")
	TObjectPtr<class USoundBase> SwingSound;

	/** 명중음 */
	UPROPERTY(EditAnywhere, Category = "MeleeWeapon|Sound")
	TObjectPtr<class USoundBase> HitSound;

	/** 명중 지점에 스폰할 파티클 */
	UPROPERTY(EditAnywhere, Category = "MeleeWeapon|Visual")
	TObjectPtr<class UParticleSystem> HitFX;

	/** 명중 파티클 크기 배율 (1 = 원본) */
	UPROPERTY(EditAnywhere, Category = "MeleeWeapon|Visual", meta = (ClampMin = "0.01"))
	float HitFXScale = 1.0f;

	/** 적중 시 카메라 셰이크 세기 (0이면 흔들지 않음) */
	UPROPERTY(EditAnywhere, Category = "MeleeWeapon|Feedback", meta = (ClampMin = "0"))
	float HitShakeScale = 0.4f;

	/** 히트스톱 길이 (실제 초). 0이면 끔 */
	UPROPERTY(EditAnywhere, Category = "MeleeWeapon|Feedback", meta = (ClampMin = "0", ClampMax = "0.5"))
	float HitStopDuration = 0.06f;

	/** 히트스톱 중 시간 배율 (0에 가까울수록 더 멈춘다) */
	UPROPERTY(EditAnywhere, Category = "MeleeWeapon|Feedback", meta = (ClampMin = "0.01", ClampMax = "1"))
	float HitStopTimeDilation = 0.05f;

private:
	/** 이번 휘두름에서 이미 맞은 액터 (한 번만 맞도록) */
	TSet<TWeakObjectPtr<AActor>> HitActors;

	/** 지난 판정 프레임의 소켓 위치 */
	FVector LastTraceLocation = FVector::ZeroVector;

	/** 판정 구간 진행 중 여부 */
	bool bIsTracing = false;

	/** 히트스톱으로 늦춘 액터와 늦추기 전 시간 배율 (시간 감속 코어가 바꾼 값을 덮어쓰지 않도록 원래 값을 기억한다) */
	TArray<TPair<TWeakObjectPtr<AActor>, float>> HitStopActors;

	FTimerHandle HitStopTimerHandle;

};


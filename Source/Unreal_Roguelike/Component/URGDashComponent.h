#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "URGDashComponent.generated.h"

/** 대시 쿨다운 변경 알림 (CooldownEndTime 쿨다운이 끝나는 월드 초, CooldownDuration 전체 쿨다운 초). 대시를 못 쓰게 되면 둘 다 0 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDelegateOnDashCooldownChanged, float, float);

/** 대시 지속 시간이 끝났음을 알림 */
DECLARE_MULTICAST_DELEGATE(FDelegateOnDashEnded);

/**
 * 대시 컴포넌트: 다리 부품이 허용한 대시의 쿨다운·방향·발사·지속 시간을 담당한다.
 * 대시를 해도 되는 상태인지(사망·피격 등)와 상태 태그·이동 입력 잠금은 소유 캐릭터가 정한다.
 * 이 컴포넌트는 끝났다는 사실만 OnDashEnded로 알린다.
 * 소유자는 ACharacter여야 한다 (LaunchCharacter와 이동 입력 벡터를 쓴다).
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_ROGUELIKE_API UURGDashComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UURGDashComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** 다리 부품이 대시를 허용하고 쿨다운이 끝났는지 검사 */
	bool CanDash() const;

	/** 대시 시작: 쿨다운을 기록·알리고, 입력 방향으로 튕겨 낸 뒤 DashDuration 뒤에 OnDashEnded. CanDash 확인은 호출자 몫 */
	void StartDash();

	/** 진행 중인 대시의 종료 예약을 취소한다 (사망 시). OnDashEnded는 오지 않는다 */
	void CancelDash();

	/** 다리 부품 반영: 대시 허용 여부와 속도·쿨다운 배율. 대시를 못 쓰게 되면 남은 쿨다운 표시를 지운다 */
	void SetLegStats(bool bInDashEnabled, float InSpeedScale, float InCooldownScale);

	/** 대시 사용·대시 불가 전환 시 호출 (HUD 다리 슬롯 쿨다운 표시용) */
	FDelegateOnDashCooldownChanged OnDashCooldownChanged;

	/** 대시 지속 시간이 끝났을 때 호출 */
	FDelegateOnDashEnded OnDashEnded;

protected:
	/** 대시 방향: 이번 프레임 입력 → 지난 프레임 입력 → 캐릭터 전방 순으로 고른 수평 단위 벡터 */
	FVector CalcDashDirection() const;

	/** 대시 종료 (타이머 콜백) → OnDashEnded */
	void HandleDashTimerExpired();

protected:
	/** 대시 속도 (cm/s). 다리 부품(FURGLegStats)이 배율을 곱한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Movement")
	float DashSpeed = 1800.0f;

	/** 대시 지속 시간 (초). 이 동안 소유자가 이동 입력을 잠근다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Movement")
	float DashDuration = 0.25f;

	/** 대시 쿨다운 (초). 다리 부품이 배율을 곱한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Cooldown")
	float DashCooldown = 1.0f;

private:
	/** 소유 캐릭터 (BeginPlay에서 한 번 캐싱) */
	UPROPERTY()
	TObjectPtr<class ACharacter> OwnerCharacter;

	/** 마지막 대시 시작 시각 (쿨다운 계산용) */
	float LastDashTime = -1.0f;

	/** 현재 다리 부품이 대시를 제공하는지. 대시는 기본 능력이 아니다 */
	bool bDashEnabled = false;

	/** 다리 부품·손상 단계가 정한 대시 속도 배율 */
	float DashSpeedScale = 1.0f;

	/** 다리 부품이 정한 대시 쿨다운 배율 */
	float DashCooldownScale = 1.0f;

	/** 대시 지속 시간 타이머 핸들 (끝나면 HandleDashTimerExpired) */
	FTimerHandle DashTimer;

};

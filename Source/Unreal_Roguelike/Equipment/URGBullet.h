#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "URGBullet.generated.h"

/** 총알이 목표 지점에 도착했을 때 알림 (무기가 이 시점에 데미지를 적용한다) */
DECLARE_DELEGATE(FDelegateOnBulletArrived);

/**
 * 코스메틱 총알(트레이서).
 * 명중 판정은 무기의 히트스캔이 발사 시점에 끝내 두고, 총알은 머즐에서 명중 지점까지 날아가는 연출과
 * 도착 시점의 착탄(명중/빗나감) 연출을 담당한다. 도착하면 OnArrived를 알려 무기가 그때 데미지를 적용한다.
 * 트레일 파티클은 BP에서 컴포넌트로 붙인다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API AURGBullet : public AActor
{
	GENERATED_BODY()

public:
	AURGBullet();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** 도착 전에 제거되면(수명 만료 등) 도착 알림을 대신 보내 데미지가 유실되지 않게 한다 */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

public:

	/**
	 * 시작 지점에서 목표 지점으로 날아가도록 발사
	 * @param bInHit 히트스캔이 무언가에 맞았는지 (착탄 연출 선택)
	 * @param InImpactNormal 착탄면 법선 (착탄 파티클 방향)
	 */
	void Launch(const FVector& InStart, const FVector& InTarget, bool bInHit, const FVector& InImpactNormal);

	/** Reuses this bullet's impact presentation for collision-driven projectiles. */
	void PlayImpactEffects(const FVector& Location, const FVector& Normal, bool bHit);
	class USoundBase* GetImpactSound() const;

protected:
	/** 직선 비행: 목표를 향해 Speed로 전진 */
	void TickStraight(float DeltaSeconds);

	/** 곡선 비행: 2차 베지어를 따라 진행하며 진행 방향으로 회전 */
	void TickArc(float DeltaSeconds);

	/** 목표 지점 도착 시 착탄 연출 후 제거 */
	void Arrive();

	/** 도착 알림을 한 번만 보낸다 */
	void NotifyArrived();

public:
	/** 목표 지점 도착 시 호출되는 델리게이트 (무기가 데미지 적용용으로 바인딩) */
	FDelegateOnBulletArrived OnArrived;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Bullet|Mesh")
	TObjectPtr<class UStaticMeshComponent> Mesh;

	/** 비행 중 총알에 붙여 재생할 트레일 파티클 (BP에서 컴포넌트로 직접 붙인 경우 비워 둔다) */
	UPROPERTY(EditAnywhere, Category = "Bullet|Trail")
	TObjectPtr<class UParticleSystem> TrailFX;

	/** 트레일 파티클 크기 배율 (1 = 원본) */
	UPROPERTY(EditAnywhere, Category = "Bullet|Trail", meta = (ClampMin = "0.01"))
	float TrailScale = 1.0f;

	/** 비행 속도 (cm/s) */
	UPROPERTY(EditAnywhere, Category = "Bullet|Movement")
	float Speed = 7000.0f;

	/** 궤적 정점 높이 (cm). 0이면 직선, 0보다 크면 위로 솟았다가 목표로 꽂히는 곡선 (미사일·유탄) */
	UPROPERTY(EditAnywhere, Category = "Bullet|Movement", meta = (ClampMin = "0"))
	float ArcHeight = 0.0f;

	/** 곡선 궤적의 좌우 무작위 편차 (cm). 여러 발이 각자 다른 궤적으로 퍼져 보이게 한다 */
	UPROPERTY(EditAnywhere, Category = "Bullet|Movement", meta = (ClampMin = "0"))
	float ArcSideSpread = 0.0f;

	/** 명중 시 착탄 지점에 스폰할 파티클 */
	UPROPERTY(EditAnywhere, Category = "Bullet|Impact")
	TObjectPtr<class UParticleSystem> ImpactFX;

	/** 빗나갔을 때 사거리 끝에 스폰할 파티클 */
	UPROPERTY(EditAnywhere, Category = "Bullet|Impact")
	TObjectPtr<class UParticleSystem> MissFX;

	/** 착탄 파티클(명중/빗나감 공통) 크기 배율 (1 = 원본) */
	UPROPERTY(EditAnywhere, Category = "Bullet|Impact", meta = (ClampMin = "0.01"))
	float ImpactScale = 1.0f;

	/** 명중음 */
	UPROPERTY(EditAnywhere, Category = "Bullet|Impact")
	TObjectPtr<class USoundBase> ImpactSound;

private:
	FVector StartLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	FVector ImpactNormal = FVector::UpVector;
	bool bIsLaunched = false;
	bool bHitTarget = false;
	bool bArrivedNotified = false;

	/** 곡선 궤적(2차 베지어) 제어점과 비행 시간. ArcHeight가 0이면 사용하지 않는다 */
	FVector ArcControlPoint = FVector::ZeroVector;
	float ArcDuration = 0.0f;
	float ArcElapsed = 0.0f;

};


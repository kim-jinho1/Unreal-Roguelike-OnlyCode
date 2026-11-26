#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "URGStageTransitionComponent.generated.h"

/**
 * 맵 이동 화면 연출. 플레이어 컨트롤러에 붙는다.
 * 도착하면 검은 화면으로 시작해 잠시 뒤 밝히고, 출구에 들어가면 그 프레임을 검게 덮고 HUD를 숨겨 로딩으로 멈춘 화면을 가린다.
 * 맵이 바뀌면 월드와 함께 타이머도 사라지므로, 덮은 뒤 타이머가 살아서 울리면 이동이 실패한 것으로 보고 화면을 되살린다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_ROGUELIKE_API UURGStageTransitionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UURGStageTransitionComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** 이동 실패 감지를 끈다. 맵을 옮기지 않고 끝나는 경우(엔딩) 덮은 검은 화면을 그대로 두기 위해 */
	void CancelTravelCover();

protected:
	/** 출구 진입: 화면을 검게 덮고 HUD를 숨긴 뒤 이동 실패 감지 타이머를 건다 */
	UFUNCTION()
	void HandleExitEntered(APawn* InPlayer);

private:
	/** 검은 화면에서 서서히 밝아진다 (도착 직후, 또는 이동이 실패해 같은 맵에 남은 경우) */
	void FadeInFromBlack() const;

protected:
	/** 맵 도착 후 검은 화면을 유지하는 시간 (초). 첫 프레임들의 셰이더 준비·스트리밍 멈춤을 가린다 */
	UPROPERTY(EditDefaultsOnly, Category = "StageTransition|Fade", meta = (ClampMin = "0.0"))
	float ArrivalFadeHoldDuration = 0.3f;

	/** 검은 화면에서 밝아지는 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "StageTransition|Fade", meta = (ClampMin = "0.0"))
	float FadeInDuration = 0.6f;

	/** 출구 진입 후 이 시간 안에 맵이 바뀌지 않으면 이동 실패로 보고 화면을 되살린다 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "StageTransition|Fade", meta = (ClampMin = "0.1"))
	float TravelCoverTimeout = 2.0f;

private:
	/** 소유 컨트롤러 (카메라 페이드·HUD 조회용) */
	UPROPERTY()
	TObjectPtr<class AURGPlayerController> OwnerController;

	/** 도착 후 검은 화면 유지 → 밝아지기 시작 타이머 */
	FTimerHandle ArrivalFadeTimer;

	/** 출구 진입 후 이동 실패 감지 타이머 */
	FTimerHandle TravelCoverTimer;

};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "URGPlayerController.generated.h"

/** 일시정지 요청 알림 (ESC). 실제 일시정지 화면은 UI 담당이 BP에서 바인딩한다 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDelegateOnPauseRequested);

/** 플레이어 사망 알림. 게임오버 화면은 컨트롤러가 직접 띄우고, 그 밖의 사망 반응은 외부에서 바인딩한다 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDelegateOnPlayerDied);

/**
 * 플레이어 컨트롤러.
 * HUD 위젯을 생성해 폰에 바인딩하고, 폰의 시스템 이벤트(일시정지 요청·사망)를 델리게이트로 외부에 알린다.
 * 사망 시 게임오버 화면을 띄우고 다시 시작·종료 요청을 게임 인스턴스에 넘긴다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API AURGPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	FORCEINLINE class UURGPlayerHUD* GetHUDWidget() const { return HUDWidget; }

	/** 폰의 ESC 입력에서 호출. OnPauseRequested를 브로드캐스트한다 */
	void RequestPause();

protected:
	/** HUD 위젯 생성 후 뷰포트에 추가 (로컬 컨트롤러, 1회) */
	void SetupHUD();

	/** 폰의 컴포넌트에 HUD와 사망 이벤트를 바인딩 (중복 바인딩 가드 포함) */
	void BindToPawn(APawn* InPawn);

	/** 플레이어 사망 (폰 AttributeComponent::OnDeath에 바인딩): 입력 모드를 UI로 전환하고 게임오버 화면 예약, OnPlayerDied 브로드캐스트 */
	void HandlePlayerDeath();

	/** 게임오버 화면 표시 */
	void ShowGameOver();

	/** 엔딩 (게임 인스턴스 OnRunCompleted에 바인딩): 이동 실패 감지를 끄고 엔딩 화면 표시 */
	void HandleRunCompleted();

	/** 결과 화면(게임오버·엔딩) 위젯 생성 후 진행도·부품 상태 표시, 위젯에 입력 포커스. 이미 결과 화면이 떠 있으면 무시 */
	void ShowResultScreen(TSubclassOf<class UURGGameOverUI> InWidgetClass);

	/** 결과 화면의 다시 시작: 게임 입력으로 되돌린 뒤 새 런 시작 */
	void HandleRetryRequested();

	/** 결과 화면의 종료: 게임 종료 */
	void HandleQuitRequested();

	/** 결과 화면의 타이틀로: 진행도를 버리고 타이틀 맵으로 이동 */
	void HandleTitleRequested();

	/** 결과 화면의 요청이 실패했을 때 화면에 갇히지 않게 버튼과 UI 입력을 되살린다 */
	void RestoreResultScreenInput();

	/** 맵의 출구마다 진입 이벤트를 구독한다 (출구는 맵에 미리 배치되어 BeginPlay 시점에 모두 존재한다) */
	void BindStageExits();

	/** 출구 진입: 맵 이동 직전에 화면을 검게 덮어 로딩으로 멈춘 프레임을 가린다 */
	UFUNCTION()
	void HandleExitEntered(APawn* InPlayer);

	/** 검은 화면에서 서서히 밝아진다 (도착 직후, 또는 이동이 실패해 같은 맵에 남은 경우) */
	void FadeInFromBlack();

public:
	/** 일시정지 요청 시 호출되는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "PlayerController|System")
	FDelegateOnPauseRequested OnPauseRequested;

	/** 플레이어 사망 시 호출되는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "PlayerController|System")
	FDelegateOnPlayerDied OnPlayerDied;

protected:
	/** HUD 위젯 클래스 (BP에서 WBP_URGPlayerHUD 지정) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerController|UI")
	TSubclassOf<class UURGPlayerHUD> HUDWidgetClass;

	/** 게임오버 위젯 클래스 (BP에서 WBP_URGGameOver 지정) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerController|UI")
	TSubclassOf<class UURGGameOverUI> GameOverWidgetClass;

	/** 엔딩 위젯 클래스 (BP에서 WBP_URGEnding 지정). 게임오버와 같은 클래스에 문구만 다르다 */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerController|UI")
	TSubclassOf<class UURGGameOverUI> EndingWidgetClass;

	/** 사망 후 게임오버 화면까지 기다리는 시간 (초). 래그돌로 쓰러지는 장면을 먼저 보여 준다 */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerController|UI", meta = (ClampMin = "0.0"))
	float GameOverDelay = 2.0f;

	/** 맵 도착 후 검은 화면을 유지하는 시간 (초). 첫 프레임들의 셰이더 준비·스트리밍 멈춤을 가린다 */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerController|Fade", meta = (ClampMin = "0.0"))
	float ArrivalFadeHoldDuration = 0.3f;

	/** 검은 화면에서 밝아지는 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerController|Fade", meta = (ClampMin = "0.0"))
	float FadeInDuration = 0.6f;

	/** 출구 진입 후 이 시간 안에 맵이 바뀌지 않으면 이동 실패로 보고 화면을 되살린다 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerController|Fade", meta = (ClampMin = "0.1"))
	float TravelCoverTimeout = 2.0f;

private:
	UPROPERTY()
	TObjectPtr<class UURGPlayerHUD> HUDWidget;

	/** 지금 떠 있는 결과 화면 (게임오버 또는 엔딩) */
	UPROPERTY()
	TObjectPtr<class UURGGameOverUI> ResultWidget;

	/** 사망 → 게임오버 화면 지연 타이머 */
	FTimerHandle GameOverTimer;

	/** 도착 후 검은 화면 유지 → 밝아지기 시작 타이머 */
	FTimerHandle ArrivalFadeTimer;

	/** 출구 진입 후 이동 실패 감지 타이머 */
	FTimerHandle TravelCoverTimer;

	/** HUD 폰 바인딩 완료 여부 (중복 초기화 방지) */
	bool bHUDInitialized = false;

	/** OnDeath 바인딩 완료 여부 (중복 바인딩 방지) */
	bool bDeathBound = false;

};


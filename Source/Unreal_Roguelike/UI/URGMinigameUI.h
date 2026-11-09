#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "URGMinigameUI.generated.h"

/** 미니게임 종료 알림 (Score 0~1) */
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateOnMinigameFinished, float);

/**
 * 미니게임 위젯 베이스. 시작·종료와 결과 통지만 담당한다.
 * 입력 모드 전환은 미니게임을 연 쪽(수리 장치 등)이 맡고, 규칙은 파생 클래스가 구현한다.
 */
UCLASS(Abstract)
class UNREAL_ROGUELIKE_API UURGMinigameUI : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 미니게임 시작: 뷰포트에 추가하고 마우스 이벤트를 받도록 가시성을 켠다. 파생 클래스는 여기서 상태를 초기화한다 */
	virtual void Start();

	FORCEINLINE bool IsRunning() const { return bIsRunning; }

public:
	/** 미니게임이 끝났을 때 호출 (점수 0~1) */
	FDelegateOnMinigameFinished OnFinished;

protected:
	/** 결과 확정: 위젯을 제거하고 OnFinished를 방송한다 */
	void Finish(float InScore);

private:
	/** 진행 중 여부 (Finish 이후 늦게 오는 타이머·입력을 무시) */
	bool bIsRunning = false;

};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "URGTitlePlayerController.generated.h"

/**
 * 타이틀 맵 전용 컨트롤러.
 * 타이틀 위젯을 띄우고 시작 요청은 게임 인스턴스의 새 런 시작으로, 종료 요청은 게임 종료로 넘긴다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API AURGTitlePlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

protected:
	/** 타이틀 화면의 시작: 게임 입력으로 되돌린 뒤 스테이지 1 무작위 맵으로 이동 */
	void HandleStartRequested();

	/** 타이틀 화면의 종료: 게임 종료 */
	void HandleQuitRequested();

protected:
	/** 타이틀 위젯 클래스 (BP에서 WBP_URGTitle 지정) */
	UPROPERTY(EditDefaultsOnly, Category = "TitlePlayerController|UI")
	TSubclassOf<class UURGTitleUI> TitleWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<class UURGTitleUI> TitleWidget;

};


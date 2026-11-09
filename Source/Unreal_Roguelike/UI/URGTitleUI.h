#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "URGTitleUI.generated.h"

/** 게임 시작 요청 알림 */
DECLARE_MULTICAST_DELEGATE(FDelegateOnStartRequested);

/** 타이틀에서 게임 종료 요청 알림 */
DECLARE_MULTICAST_DELEGATE(FDelegateOnTitleQuitRequested);

/**
 * 게임을 처음 켰을 때 뜨는 타이틀 화면. 제목과 시작·종료 버튼을 보여 준다.
 * 실제 맵 이동·종료는 하지 않고 델리게이트로 컨트롤러에 알린다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGTitleUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

public:
	/** 시작 버튼을 눌렀을 때 호출 */
	FDelegateOnStartRequested OnStartRequested;

	/** 종료 버튼을 눌렀을 때 호출 */
	FDelegateOnTitleQuitRequested OnQuitRequested;

protected:
	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleQuitClicked();

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UButton> StartButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UButton> QuitButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> TitleText;

	/** 화면에 표시할 게임 제목 */
	UPROPERTY(EditDefaultsOnly, Category = "TitleUI|Text")
	FText TitleLabel = NSLOCTEXT("URG.TitleUI", "Title", "UNREAL ROGUELIKE");

};


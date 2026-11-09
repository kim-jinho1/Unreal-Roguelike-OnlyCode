#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "URGCommLogUI.generated.h"

/**
 * 관제 AI 통신 텍스트 UI (중앙 상단). 상황 발생 시만 표시되고 일정 시간 후 자동으로 숨는다
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGCommLogUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	/** 메시지를 표시하고 InDuration 초 후 숨긴다 (0 이하이면 수동으로 숨길 때까지 유지) */
	void ShowMessage(const FText& InMessage, float InDuration);

	/** 메시지 즉시 숨김 */
	void HideMessage();

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> MessageText;

private:
	/** 자동 숨김 타이머 */
	FTimerHandle HideTimer;

};


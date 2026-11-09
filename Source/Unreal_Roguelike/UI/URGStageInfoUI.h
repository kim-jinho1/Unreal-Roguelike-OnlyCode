#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "URGStageInfoUI.generated.h"

/**
 * 현재 스테이지와 진행한 방 개수를 표시하는 UI (우측 상단)
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGStageInfoUI : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 스테이지/방 텍스트 갱신 */
	void SetStageInfo(int32 InStage, int32 InRoom, int32 InTotalRooms) const;

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> StageText;

private:
	/** 스테이지 텍스트 포맷 ({0} = 스테이지, {1} = 진행한 방, {2} = 전체 방) */
	UPROPERTY(EditDefaultsOnly, Category = "StageInfoUI|Text")
	FText StageTextFormat = NSLOCTEXT("URG.StageInfoUI", "StageFormat", "Stage {0}   Room {1} / {2}");

};


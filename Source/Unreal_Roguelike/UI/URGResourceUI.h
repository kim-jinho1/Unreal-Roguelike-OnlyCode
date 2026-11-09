#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "URGResourceUI.generated.h"

/**
 * 보유 자원 수치를 표시하는 UI
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGResourceUI : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 자원 텍스트 갱신 */
	void SetResource(int32 InResource) const;

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> ResourceText;

private:
	/** 자원 텍스트 포맷 ({0} = 보유량) */
	UPROPERTY(EditDefaultsOnly, Category = "ResourceUI|Text")
	FText ResourceTextFormat = NSLOCTEXT("URG.ResourceUI", "ResourceFormat", "Resource : {0}");

};


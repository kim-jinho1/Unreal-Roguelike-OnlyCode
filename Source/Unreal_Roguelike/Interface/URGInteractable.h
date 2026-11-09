#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "URGInteractable.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UURGInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 플레이어가 E 키로 상호작용할 수 있는 액터의 계약.
 * 부품 픽업·수리 장치·상점 등은 이 인터페이스만 구현하면 되고, 탐색은 플레이어의 InteractTrace 라인트레이스가 담당한다.
 * C++/BP 양쪽에서 구현할 수 있도록 BlueprintNativeEvent로 선언한다.
 */
class IURGInteractable
{
	GENERATED_BODY()

public:
	/** 상호작용 실행. InInstigator는 상호작용을 시도한 액터(플레이어) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable")
	void Interact(AActor* InInstigator);

	/** HUD에 표시할 상호작용 안내 문구 (예: "부품 줍기") */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable")
	FText GetInteractText() const;

	/** 현재 상호작용 가능한지 여부 (기본 true) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable")
	bool CanInteract(AActor* InInstigator) const;

};

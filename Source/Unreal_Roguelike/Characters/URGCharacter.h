#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "URGDefine.h"
#include "URGCharacter.generated.h"

/**
 * 플레이어·적 공통 캐릭터 베이스.
 * 체력(AttributeComponent)과 상태(StateComponent)를 보유하며,
 * 사망은 AttributeComponent::OnDeath → HandleDeath()로 일원화한다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API AURGCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AURGCharacter();

protected:
	virtual void BeginPlay() override;

public:
	FORCEINLINE class UURGAttributeComponent* GetAttributeComponent() const { return AttributeComponent; }
	FORCEINLINE class UURGStateComponent* GetStateComponent() const { return StateComponent; }

	/** 사망 상태 여부 (StateComponent 기준) */
	bool IsDead() const;

protected:
	/** 사망 처리 (AttributeComponent::OnDeath에 바인딩). 파생 클래스는 연출을 덧붙이고 Super를 호출한다 */
	virtual void HandleDeath();

protected:
	/** 체력 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Components")
	TObjectPtr<class UURGAttributeComponent> AttributeComponent;

	/** 상태(GameplayTag) 및 이동 입력 잠금 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Components")
	TObjectPtr<class UURGStateComponent> StateComponent;

};

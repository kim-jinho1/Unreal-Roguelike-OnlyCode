#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "URGPlayerInputComponent.generated.h"

/**
 * 플레이어 입력 관리 컴포넌트.
 * 매핑 컨텍스트 등록과 Enhanced Input 액션 → 캐릭터 함수 바인딩만 담당한다.
 * 액션 애셋은 BP_URGPlayerCharacter에서 지정한다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_ROGUELIKE_API UURGPlayerInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UURGPlayerInputComponent();

public:
	/** APawn::SetupPlayerInputComponent에서 호출 — Enhanced Input 액션을 소유 캐릭터에 바인딩 */
	void BindInputActions(class UInputComponent* PlayerInputComponent);

	/** NotifyControllerChanged에서 호출 — 로컬 플레이어에 기본 매핑 컨텍스트를 추가 */
	void AddMappingContext();

protected:
	UPROPERTY(EditAnywhere, Category = "PlayerInput|Context")
	TObjectPtr<class UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "PlayerInput|Action")
	TObjectPtr<class UInputAction> MoveAction;			// 이동 (WASD)

	UPROPERTY(EditAnywhere, Category = "PlayerInput|Action")
	TObjectPtr<class UInputAction> LookAction;			// 시점 이동 (마우스)

	UPROPERTY(EditAnywhere, Category = "PlayerInput|Action")
	TObjectPtr<class UInputAction> JumpAction;			// 점프 (Space)

	UPROPERTY(EditAnywhere, Category = "PlayerInput|Action")
	TObjectPtr<class UInputAction> DashAction;			// 대시 (Shift)

	UPROPERTY(EditAnywhere, Category = "PlayerInput|Action")
	TObjectPtr<class UInputAction> FireAction;			// 발사 (LMB)

	UPROPERTY(EditAnywhere, Category = "PlayerInput|Action")
	TObjectPtr<class UInputAction> AimAction;			// 조준 (RMB)

	UPROPERTY(EditAnywhere, Category = "PlayerInput|Action")
	TObjectPtr<class UInputAction> ReloadAction;		// 재장전 (R)

	UPROPERTY(EditAnywhere, Category = "PlayerInput|Action")
	TObjectPtr<class UInputAction> InteractAction;		// 상호작용 (E)

	UPROPERTY(EditAnywhere, Category = "PlayerInput|Action")
	TObjectPtr<class UInputAction> CoreAbilityAction;	// 코어 특수 능력 (Q)

	UPROPERTY(EditAnywhere, Category = "PlayerInput|Action")
	TObjectPtr<class UInputAction> PauseAction;			// 일시정지 (ESC)

};

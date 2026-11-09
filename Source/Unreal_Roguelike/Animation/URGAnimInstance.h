#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "URGAnimInstance.generated.h"

/**
 * 캐릭터 공용 애님 인스턴스.
 * 이동/낙하/방향 데이터와 조준 각도를 매 프레임 갱신해 ABP에 노출한다.
 * 조준·대시 여부는 소유 캐릭터의 컴포넌트(Aim/State)에서 직접 읽으므로
 * 플레이어·적 어느 쪽에 붙여도 동작한다 (컴포넌트가 없으면 false).
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UURGAnimInstance();

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:
	/** 몽타주 노티파이: 이동 입력 재활성화 */
	UFUNCTION()
	void AnimNotify_ResetMovementInput();

	/** 몽타주 노티파이: 현재 상태 초기화 */
	UFUNCTION()
	void AnimNotify_ResetCurrentState();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|References")
	TObjectPtr<class ACharacter> Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|References")
	TObjectPtr<class UCharacterMovementComponent> MovementComponent;

	/** 조준 여부 조회용 (없으면 항상 비조준) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|References")
	TObjectPtr<class UURGAimComponent> AimComponent;

	/** 상태 조회용 (없으면 항상 비대시) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|References")
	TObjectPtr<class UURGStateComponent> StateComponent;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Anim|Movement")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Anim|Movement")
	float GroundSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Anim|Movement")
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "Anim|Movement")
	bool bIsFalling = false;

	/** 이동 방향 각도 (-180~180, 0=전방 90=우 -90=좌). 스트레이프 블렌드스페이스용 */
	UPROPERTY(BlueprintReadOnly, Category = "Anim|Movement")
	float Direction = 0.0f;

	/** 대시 중 여부 (Travel_Mode 연출용) */
	UPROPERTY(BlueprintReadOnly, Category = "Anim|Movement")
	bool bIsDashing = false;

	/** 조준 중 여부 (조준 포즈/Aim Offset 블렌딩용) */
	UPROPERTY(BlueprintReadOnly, Category = "Anim|Combat")
	bool bIsAiming = false;

	/** 조준 상하 각도 (Aim Offset의 Pitch 입력용, -90~90) */
	UPROPERTY(BlueprintReadOnly, Category = "Anim|Combat")
	float AimPitch = 0.0f;

	/** 조준 좌우 각도 (Aim Offset의 Yaw 입력용, -90~90로 접음) */
	UPROPERTY(BlueprintReadOnly, Category = "Anim|Combat")
	float AimYaw = 0.0f;

};

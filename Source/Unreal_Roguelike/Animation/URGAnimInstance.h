#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "URGAnimInstance.generated.h"

/**
 * 캐릭터 공용 애님 인스턴스.
 * 이동/낙하/방향 데이터와 조준 각도를 매 프레임 갱신해 ABP에 노출한다.
 * 조준 여부는 소유 캐릭터의 AimComponent에서 직접 읽으므로
 * 플레이어·적 어느 쪽에 붙여도 동작한다 (컴포넌트가 없으면 비조준).
 * 캐릭터 상태는 바꾸지 않는다. 몽타주가 끝났을 때의 상태 해제는 몽타주를 튼 쪽이 맡는다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|References")
	TObjectPtr<class ACharacter> Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|References")
	TObjectPtr<class UCharacterMovementComponent> MovementComponent;

	/** 조준 여부 조회용 (없으면 항상 비조준) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|References")
	TObjectPtr<class UURGAimComponent> AimComponent;

protected:
	/** 수평 속력 (cm/s). 적 애님 인스턴스가 이동 판정과 재생 속도에 쓴다 */
	UPROPERTY(BlueprintReadOnly, Category = "Anim|Movement")
	float GroundSpeed = 0.0f;

	/** 이동 입력이 있고 실제로 움직이는 중 (정지/이동 스테이트 전환용) */
	UPROPERTY(BlueprintReadOnly, Category = "Anim|Movement")
	bool bShouldMove = false;

	/** 공중에 있는지 (점프·낙하 스테이트용) */
	UPROPERTY(BlueprintReadOnly, Category = "Anim|Movement")
	bool bIsFalling = false;

	/** 몸 기준 이동 방향 각도 (-180~180, 0=전방 90=우 -90=좌). 조준 중에만 계산하고 그 밖에는 0. 스트레이프 블렌드스페이스용 */
	UPROPERTY(BlueprintReadOnly, Category = "Anim|Movement")
	float Direction = 0.0f;

	/** 조준 중 여부. Direction 계산 여부를 가른다 */
	UPROPERTY(BlueprintReadOnly, Category = "Anim|Combat")
	bool bIsAiming = false;

	/** 조준 상하 각도 (Aim Offset의 Pitch 입력용, -90~90) */
	UPROPERTY(BlueprintReadOnly, Category = "Anim|Combat")
	float AimPitch = 0.0f;

	/** 조준 좌우 각도 (Aim Offset의 Yaw 입력용, 90을 넘으면 앞쪽으로 접어 -90~90) */
	UPROPERTY(BlueprintReadOnly, Category = "Anim|Combat")
	float AimYaw = 0.0f;

};

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "URGAimComponent.generated.h"

/**
 * 조준(RMB) 상태 관리 컴포넌트.
 * 조준 중에는 카메라 거리·어깨 오프셋·FOV를 보간하고, 이동 중 몸통이 항상 시점 방향을 향하게 한다.
 * 제자리에서는 조준 여부와 무관하게 상체(Aim Offset)만 시점을 따라가다가
 * 임계 각도를 넘으면 몸통을 시점 방향으로 돌린다. 비조준 이동 중에는 이동 방향 회전을 사용한다.
 * 벽에 막혀 카메라가 몸에 붙으면 캐릭터를 숨긴다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_ROGUELIKE_API UURGAimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UURGAimComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	/** 현재 조준 중인지 여부 */
	UFUNCTION(BlueprintPure, Category = "Aim")
	FORCEINLINE bool IsAiming() const { return bIsAiming; }

	/** 조준 시작 */
	void Aim();

	/** 조준 종료 */
	void StopAim();

private:
	/** 조준 상태에 따라 카메라 거리/어깨 오프셋을 부드럽게 보간 */
	void UpdateCameraForAim(float DeltaTime);

	/** 몸통 Yaw를 시점 방향으로 회전 (제자리: 임계 각도 초과 시 / 조준 이동 중: 상시) */
	void UpdateBodyRotation(float DeltaTime);

	/** 카메라가 캐릭터에 너무 붙으면 캐릭터 메시(붙은 무기 포함)를 숨기고, 멀어지면 다시 보인다 */
	void UpdateCharacterVisibility();

private:
	/** 조준 시 카메라 거리 */
	UPROPERTY(EditAnywhere, Category = "Aim|Camera")
	float AimArmLength = 200.0f;

	/** 조준 시 카메라 어깨(소켓) 오프셋. */
	UPROPERTY(EditAnywhere, Category = "Aim|Camera")
	FVector AimCameraSocketOffset = FVector(0.0f, 90.0f, 55.0f);

	/** 비조준 시 카메라 어깨(소켓) 오프셋. */
	UPROPERTY(EditAnywhere, Category = "Aim|Camera")
	FVector NormalCameraSocketOffset = FVector(0.0f, 40.0f, 20.0f);

	/** 조준 카메라 전환 보간 속도 */
	UPROPERTY(EditAnywhere, Category = "Aim|Camera")
	float AimCameraInterpSpeed = 12.0f;

	/** 조준 시 시야각(FOV) */
	UPROPERTY(EditAnywhere, Category = "Aim|Camera")
	float AimFieldOfVision = 60.0f;

	/** 카메라와 캐릭터 사이 거리(cm)가 이보다 가까우면 캐릭터를 숨긴다. 벽에 막혀 카메라가 등에 붙을 때 시야 확보용 */
	UPROPERTY(EditAnywhere, Category = "Aim|Camera", meta = (ClampMin = "0"))
	float HideCharacterDistance = 120.0f;
	/** 제자리에서 몸통을 돌리기 시작하는 시점-몸통 Yaw 차이 (도) */
	UPROPERTY(EditAnywhere, Category = "Aim|Rotation", meta = (ClampMin = "0", ClampMax = "90"))
	float TurnInPlaceThreshold = 90.0f;

	/** 시점 방향으로의 몸통 회전 속도 (도/초) */
	UPROPERTY(EditAnywhere, Category = "Aim|Rotation", meta = (ClampMin = "0"))
	float TurnRate = 540.0f;

private:
	/** 소유 캐릭터 및 카메라 관련 컴포넌트 캐시 (BeginPlay에서 설정) */
	TObjectPtr<class ACharacter> OwnerCharacter;

	TObjectPtr<class USpringArmComponent> SpringArm;

	TObjectPtr<class UCameraComponent> Camera;

	/** 조준 해제 시 복귀할 평상시 카메라 거리/시야각 (BeginPlay 초기값에서 캡처) */
	float NormalArmLength = 300.0f;

	float NormalFieldOfVision = 90.0f;

	/** 조준 해제 시 복귀할 평상시 카메라 랙 사용 여부 (조준 중에는 항상 끔) */
	bool bNormalCameraLag = true;

	/** 현재 조준 중 여부 */
	bool bIsAiming = false;

	/** 제자리 몸통 회전 진행 중 여부 (임계 각도를 넘어 시작되면 시점과 정렬될 때까지 유지) */
	bool bIsTurningInPlace = false;

	/** 카메라 근접으로 캐릭터를 숨긴 상태인지 (상태가 바뀔 때만 가시성을 갱신하기 위해 기록) */
	bool bIsCharacterHiddenByCamera = false;
};

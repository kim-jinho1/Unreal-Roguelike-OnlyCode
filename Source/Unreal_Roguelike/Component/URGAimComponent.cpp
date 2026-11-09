#include "Component/URGAimComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"

UURGAimComponent::UURGAimComponent()
{
	// 카메라 보간과 몸통 회전을 위해 매 프레임 Tick이 필요하다.
	PrimaryComponentTick.bCanEverTick = true;
}

void UURGAimComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		SpringArm = OwnerCharacter->FindComponentByClass<USpringArmComponent>();
		Camera = OwnerCharacter->FindComponentByClass<UCameraComponent>();
	}

	// 조준 해제 시 복귀할 평상시 값을 초기 상태에서 캡처한다.
	if (SpringArm)
	{
		NormalArmLength = SpringArm->TargetArmLength;
		bNormalCameraLag = SpringArm->bEnableCameraLag;

		// 시작 직후 0에서 보간되며 카메라가 움직이지 않도록 평상시 오프셋을 즉시 적용한다.
		SpringArm->SocketOffset = NormalCameraSocketOffset;
	}

	if (Camera)
	{
		NormalFieldOfVision = Camera->FieldOfView;
	}
}

void UURGAimComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateCameraForAim(DeltaTime);
	UpdateBodyRotation(DeltaTime);
	UpdateCharacterVisibility();
}

void UURGAimComponent::UpdateCameraForAim(float DeltaTime)
{
	if (SpringArm == nullptr)
	{
		return;
	}

	const FVector TargetSocketOffset = bIsAiming ? AimCameraSocketOffset : NormalCameraSocketOffset;
	const float TargetArmLength = bIsAiming ? AimArmLength : NormalArmLength;

	SpringArm->SocketOffset = FMath::VInterpTo(SpringArm->SocketOffset, TargetSocketOffset, DeltaTime, AimCameraInterpSpeed);
	SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, TargetArmLength, DeltaTime, AimCameraInterpSpeed);
}

void UURGAimComponent::UpdateBodyRotation(float DeltaTime)
{
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	// 비조준 이동 중에는 이동 방향 회전(bOrientRotationToMovement)에 맡긴다.
	const bool bIsMoving = OwnerCharacter->GetVelocity().SizeSquared2D() > 1.0f;
	if (!bIsAiming && bIsMoving)
	{
		bIsTurningInPlace = false;
		return;
	}

	const FRotator ActorRotation = OwnerCharacter->GetActorRotation();
	const float ControlYaw = OwnerCharacter->GetControlRotation().Yaw;
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(ActorRotation.Yaw, ControlYaw);

	if (!bIsMoving && !bIsTurningInPlace && FMath::Abs(DeltaYaw) < TurnInPlaceThreshold)
	{
		return;
	}

	FRotator NewRotation = ActorRotation;
	NewRotation.Yaw = FMath::FixedTurn(ActorRotation.Yaw, ControlYaw, TurnRate * DeltaTime);
	OwnerCharacter->SetActorRotation(NewRotation);

	bIsTurningInPlace = !FMath::IsNearlyZero(FMath::FindDeltaAngleDegrees(NewRotation.Yaw, ControlYaw), 0.1f);
}

void UURGAimComponent::UpdateCharacterVisibility()
{
	if (OwnerCharacter == nullptr || Camera == nullptr)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (Mesh == nullptr)
	{
		return;
	}

	// 벽에 막혀 스프링암이 줄어들면 카메라가 캐릭터 등에 붙어 몸이 화면을 덮는다. 이때는 캐릭터보다 시야를 우선한다.
	const float DistanceSquared = FVector::DistSquared(Camera->GetComponentLocation(), OwnerCharacter->GetActorLocation());
	const bool bShouldHide = DistanceSquared < FMath::Square(HideCharacterDistance);
	if (bShouldHide == bIsCharacterHiddenByCamera)
	{
		return;
	}

	bIsCharacterHiddenByCamera = bShouldHide;

	// 손에 든 무기는 메시 소켓에 붙은 자식이라 자식까지 같이 숨긴다
	Mesh->SetVisibility(!bShouldHide, true);
}

void UURGAimComponent::Aim()
{
	if (bIsAiming || OwnerCharacter == nullptr)
	{
		return;
	}

	bIsAiming = true;

	if (Camera)
	{
		Camera->FieldOfView = AimFieldOfVision;
	}

	// 조준 중 이동 시 카메라가 뒤처지면 캐릭터가 화면 중앙(크로스헤어) 쪽으로 밀려 들어오므로 랙을 끈다.
	if (SpringArm)
	{
		SpringArm->bEnableCameraLag = false;
	}

	// 조준 중에는 이동 방향 회전을 끄고 몸통 회전을 UpdateBodyRotation이 직접 제어한다.
	if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
	}
}

void UURGAimComponent::StopAim()
{
	if (!bIsAiming || OwnerCharacter == nullptr)
	{
		return;
	}

	bIsAiming = false;

	if (Camera)
	{
		Camera->FieldOfView = NormalFieldOfVision;
	}

	if (SpringArm)
	{
		SpringArm->bEnableCameraLag = bNormalCameraLag;
	}

	if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = true;
	}
}

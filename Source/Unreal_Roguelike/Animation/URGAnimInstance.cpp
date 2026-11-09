#include "Animation/URGAnimInstance.h"
#include "Component/URGAimComponent.h"
#include "Component/URGStateComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "URGGameplayTags.h"

UURGAnimInstance::UURGAnimInstance()
{
}

void UURGAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<ACharacter>(TryGetPawnOwner());
	if (Character)
	{
		MovementComponent = Character->GetCharacterMovement();
		AimComponent = Character->FindComponentByClass<UURGAimComponent>();
		StateComponent = Character->FindComponentByClass<UURGStateComponent>();
	}
}

void UURGAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (Character == nullptr || MovementComponent == nullptr)
	{
		return;
	}

	Velocity = MovementComponent->Velocity;
	GroundSpeed = Velocity.Size2D();
	bShouldMove = GroundSpeed > 3.0f && MovementComponent->GetCurrentAcceleration() != FVector::ZeroVector;
	bIsFalling = MovementComponent->IsFalling();

	bIsAiming = AimComponent ? AimComponent->IsAiming() : false;
	bIsDashing = StateComponent ? StateComponent->IsCurrentStateEqualTo(URGGameplayTags::Character_State_Dashing) : false;

	if (bIsAiming && GroundSpeed > 1.0f)
	{
		const FVector LocalVelocity = Character->GetActorRotation().UnrotateVector(Velocity);
		Direction = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
	}
	else
	{
		Direction = 0.0f;
	}

	const FRotator AimDelta = (Character->GetBaseAimRotation() - Character->GetActorRotation()).GetNormalized();
	const float AbsAimYaw = FMath::Abs(AimDelta.Yaw);
	AimPitch = AimDelta.Pitch;
	AimYaw = AbsAimYaw <= 90.0f ? AimDelta.Yaw : FMath::Sign(AimDelta.Yaw) * (180.0f - AbsAimYaw);
}

void UURGAnimInstance::AnimNotify_ResetMovementInput()
{
	if (StateComponent)
	{
		StateComponent->ToggleMovementInput(true);
	}
}

void UURGAnimInstance::AnimNotify_ResetCurrentState()
{
	if (StateComponent)
	{
		StateComponent->ClearCurrentState();
	}
}

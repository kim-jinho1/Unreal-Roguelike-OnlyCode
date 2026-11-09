#include "Component/URGPlayerInputComponent.h"
#include "Characters/URGPlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

UURGPlayerInputComponent::UURGPlayerInputComponent()
{
	// 바인딩만 담당하므로 매 프레임 Tick은 필요하지 않다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UURGPlayerInputComponent::BindInputActions(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	AURGPlayerCharacter* Character = Cast<AURGPlayerCharacter>(GetOwner());

	if (EnhancedInputComponent == nullptr || Character == nullptr)
	{
		return;
	}

	// Movement
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, Character, &AURGPlayerCharacter::Input_Move);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, Character, &AURGPlayerCharacter::Input_Look);

	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, Character, &AURGPlayerCharacter::Input_Jump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, Character, &AURGPlayerCharacter::Input_StopJump);

	EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, Character, &AURGPlayerCharacter::Input_Dash);

	// Combat
	EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, Character, &AURGPlayerCharacter::Input_SingleAttack);
	EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, Character, &AURGPlayerCharacter::Input_RapidAttack);

	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, Character, &AURGPlayerCharacter::Input_Aim);
	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, Character, &AURGPlayerCharacter::Input_StopAim);

	EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, Character, &AURGPlayerCharacter::Input_Reload);

	// Interaction / Ability / System
	EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, Character, &AURGPlayerCharacter::Input_Interact);
	EnhancedInputComponent->BindAction(CoreAbilityAction, ETriggerEvent::Started, Character, &AURGPlayerCharacter::Input_CoreAbility);
	EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, Character, &AURGPlayerCharacter::Input_Pause);
}

void UURGPlayerInputComponent::AddMappingContext()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(OwnerPawn->GetController());
	if (PlayerController == nullptr)
	{
		return;
	}

	auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	if (Subsystem)
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

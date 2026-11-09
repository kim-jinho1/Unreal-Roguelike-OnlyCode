#include "Characters/URGCharacter.h"
#include "Component/URGAttributeComponent.h"
#include "Component/URGStateComponent.h"
#include "Core/URGGameMode.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "URGGameplayTags.h"

AURGCharacter::AURGCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 컨트롤러 회전은 사용하지 않고, 이동 방향으로 메시가 회전하게 한다.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->bOrientRotationToMovement = true;
	}

	// 캐릭터가 카메라 스프링암과 캐릭터 사이를 지나가도 카메라가 당겨지지 않게 카메라 채널은 통과시킨다.
	// 카메라가 막혀야 하는 것은 벽 같은 지형뿐이다.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	AttributeComponent = CreateDefaultSubobject<UURGAttributeComponent>(TEXT("Attribute"));
	StateComponent = CreateDefaultSubobject<UURGStateComponent>(TEXT("State"));
}

void AURGCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AttributeComponent)
	{
		AttributeComponent->OnDeath.AddUObject(this, &AURGCharacter::HandleDeath);
	}
}

void AURGCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AURGCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

bool AURGCharacter::IsDead() const
{
	if (StateComponent == nullptr)
	{
		return false;
	}

	return StateComponent->IsCurrentStateEqualTo(URGGameplayTags::Character_State_Death);
}

void AURGCharacter::HandleDeath()
{
	// 사망 상태는 이후 어떤 상태로도 덮어써지지 않는다.
	if (StateComponent)
	{
		StateComponent->SetCurrentState(URGGameplayTags::Character_State_Death);
		StateComponent->ToggleMovementInput(false);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 사망 시 충돌 비활성화
	}

	// 사격·폭발·화상 어떤 이유로 죽어도 떨어지도록 공통 사망 처리에서 요청한다. 무엇을 떨어뜨릴지는 게임모드가 정한다
	if (!IsPlayerControlled())
	{
		const UWorld* World = GetWorld();
		if (const AURGGameMode* GameMode = World ? World->GetAuthGameMode<AURGGameMode>() : nullptr)
		{
			GameMode->SpawnResourceDrop(GetActorLocation());
		}
	}
}

EURGHitDirection AURGCharacter::CalcHitDirection(const FVector& InSourceLocation) const
{
	const FVector ToSource = (InSourceLocation - GetActorLocation()).GetSafeNormal2D();
	const float Fwd = FVector::DotProduct(GetActorForwardVector(), ToSource);
	const float Rgt = FVector::DotProduct(GetActorRightVector(), ToSource);

	// 앞뒤 성분이 좌우 성분보다 크면 앞뒤 축, 아니면 좌우 축. 부호로 방향을 가른다.
	if (FMath::Abs(Fwd) >= FMath::Abs(Rgt))
	{
		return Fwd >= 0.0f ? EURGHitDirection::Front : EURGHitDirection::Back;
	}

	return Rgt >= 0.0f ? EURGHitDirection::Right : EURGHitDirection::Left;
}

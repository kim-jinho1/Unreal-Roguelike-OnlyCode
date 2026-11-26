#include "Animation/URGAnimInstance.h"
#include "Component/URGAimComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** 이 수평 속력(cm/s)을 넘어야 움직이는 중으로 본다. 멈춘 뒤 남는 미세한 속도에 이동 포즈가 켜지지 않게 한다 */
	constexpr float MoveSpeedThreshold = 3.0f;

	/** 방향 계산 최소 속력(cm/s). 거의 멈춘 속도는 방향이 크게 흔들려 블렌드스페이스가 떨린다 */
	constexpr float DirectionSpeedThreshold = 1.0f;

	/** 조준 오프셋이 받는 좌우 최대 각도 */
	constexpr float MaxAimYaw = 90.0f;
}

void UURGAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// 주인이 캐릭터가 아니면(애님 미리보기 등) 참조를 비워 두고, 매 프레임 갱신은 기본값을 유지한다
	Character = Cast<ACharacter>(TryGetPawnOwner());
	if (Character == nullptr)
	{
		return;
	}

	MovementComponent = Character->GetCharacterMovement();
	AimComponent = Character->FindComponentByClass<UURGAimComponent>();
}

void UURGAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (Character == nullptr || MovementComponent == nullptr)
	{
		return;
	}

	const FVector Velocity = MovementComponent->Velocity;
	GroundSpeed = Velocity.Size2D();
	// 가속(입력)까지 봐서, 키를 놓고 미끄러지며 감속하는 동안에는 바로 정지 포즈로 넘어가게 한다
	bShouldMove = GroundSpeed > MoveSpeedThreshold && MovementComponent->GetCurrentAcceleration() != FVector::ZeroVector;
	bIsFalling = MovementComponent->IsFalling();
	bIsAiming = AimComponent != nullptr && AimComponent->IsAiming();

	// 비조준 이동은 몸이 이동 방향을 보므로(bOrientRotationToMovement) 몸 기준으로 늘 전방이다.
	// 조준 중에만 몸이 카메라를 보며 옆·뒤로 걸으므로 몸 기준 이동 각도가 필요하다.
	if (bIsAiming && GroundSpeed > DirectionSpeedThreshold)
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
	// 차이가 90도를 넘는 건 비조준으로 카메라 쪽을 향해 달릴 때뿐이다 (조준 중엔 몸이 카메라를 따라 돌고, 서 있을 땐 제자리 회전한다).
	// 그대로 넣으면 고개가 등 뒤로 꺾이므로 앞쪽으로 접는다: 120도 → 60도, 180도 → 0도.
	AimYaw = AbsAimYaw <= MaxAimYaw ? AimDelta.Yaw : FMath::Sign(AimDelta.Yaw) * (180.0f - AbsAimYaw);
}

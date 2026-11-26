#include "Component/URGHitReactComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UURGHitReactComponent::UURGHitReactComponent()
{
	// 피격·사망 시에만 호출되는 이벤트 기반 연출이므로 매 프레임 Tick은 필요하지 않다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UURGHitReactComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	check(OwnerCharacter);
}

bool UURGHitReactComponent::PlayHitReact(float InDamageRatio, const AActor* InCauser)
{
	// 리액션 몽타주가 없는 방향으로 맞아도 소리는 나야 하므로 몽타주 검사보다 먼저 재생한다
	PlayPainSound(InDamageRatio);

	// 가해자를 모르면 정면 피격으로 본다.
	const EURGHitDirection Direction = InCauser ? CalcHitDirection(InCauser->GetActorLocation()) : EURGHitDirection::Front;

	// 몽타주가 없는 방향은 리액션을 생략한다. 소유자는 false를 보고 상태 진입도 생략한다 (풀어 줄 계기가 없으므로).
	UAnimMontage* Montage = HitReactMontages.FindRef(Direction);
	if (Montage == nullptr)
	{
		return false;
	}

	OwnerCharacter->PlayAnimMontage(Montage);

	// 종료는 몽타주 종료 델리게이트로 맞춘다 (애셋 노티파이에 의존하지 않는다).
	if (UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UURGHitReactComponent::HandleMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
	}

	return true;
}

void UURGHitReactComponent::PlayDeath(const AActor* InCauser)
{
	USkeletalMeshComponent* MeshComp = OwnerCharacter->GetMesh();
	if (MeshComp == nullptr)
	{
		return;
	}

	// 메시에 붙여 래그돌로 날아가도 비명이 몸을 따라가게 한다
	if (DeathSound)
	{
		UGameplayStatics::SpawnSoundAttached(DeathSound, MeshComp);
	}

	// Death 애님은 튕겨 오르는 구간까지만 있어(래그돌 인계용) 물리로 넘겨 바닥에 눕힌다.
	MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
	MeshComp->SetSimulatePhysics(true);

	// 마지막 타격 방향으로 밀려 나가며 쓰러지게 한다. 가해자를 모르면 제자리에서 쓰러진다.
	if (InCauser == nullptr)
	{
		return;
	}

	FVector PushDirection = (OwnerCharacter->GetActorLocation() - InCauser->GetActorLocation()).GetSafeNormal2D();
	PushDirection = (PushDirection + FVector::UpVector * DeathImpulseUpRatio).GetSafeNormal();
	// 래그돌은 바디가 여러 개라 하나만 밀면 나머지에 붙잡힌다. 전체 바디에 같은 속도 변화를 준다.
	MeshComp->AddImpulseToAllBodiesBelow(PushDirection * DeathImpulse, NAME_None, true); // bVelChange: 질량과 무관
}

void UURGHitReactComponent::PlayPainSound(float InDamageRatio)
{
	const bool bHeavy = InDamageRatio >= PainHeavyDamageRatio;
	const float Now = GetWorld()->GetTimeSeconds();

	// 큰 피해는 확실히 알려야 하므로 간격 제한을 무시한다
	if (!bHeavy && Now - LastPainSoundTime < PainSoundInterval)
	{
		return;
	}

	USoundBase* Sound = bHeavy ? PainHeavySound.Get() : PainSound.Get();
	if (Sound == nullptr)
	{
		return;
	}

	// 메시에 붙여 캐릭터가 밀려나도 소리가 몸을 따라가게 한다
	UGameplayStatics::SpawnSoundAttached(Sound, OwnerCharacter->GetMesh());
	LastPainSoundTime = Now;
}

EURGHitDirection UURGHitReactComponent::CalcHitDirection(const FVector& InSourceLocation) const
{
	const FVector ToSource = (InSourceLocation - OwnerCharacter->GetActorLocation()).GetSafeNormal2D();
	const float Fwd = FVector::DotProduct(OwnerCharacter->GetActorForwardVector(), ToSource);
	const float Rgt = FVector::DotProduct(OwnerCharacter->GetActorRightVector(), ToSource);

	// 앞뒤 성분이 좌우 성분보다 크면 앞뒤 축, 아니면 좌우 축. 부호로 방향을 가른다.
	if (FMath::Abs(Fwd) >= FMath::Abs(Rgt))
	{
		return Fwd >= 0.0f ? EURGHitDirection::Front : EURGHitDirection::Back;
	}

	return Rgt >= 0.0f ? EURGHitDirection::Right : EURGHitDirection::Left;
}

void UURGHitReactComponent::HandleMontageEnded(UAnimMontage* InMontage, bool bInterrupted)
{
	OnHitReactEnded.Broadcast(bInterrupted);
}

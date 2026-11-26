#include "Component/URGInteractComponent.h"
#include "Interface/URGInteractable.h"
#include "URGDefine.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

UURGInteractComponent::UURGInteractComponent()
{
	// 포커스는 InteractFocusInterval 타이머로만 갱신하므로 매 프레임 Tick은 필요하지 않다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UURGInteractComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPawn = Cast<APawn>(GetOwner());
	check(OwnerPawn);

	// 바라보는 상호작용 대상(부품 비교 UI 등)은 매 프레임이 아니라 짧은 간격으로만 찾는다
	GetWorld()->GetTimerManager().SetTimer(InteractFocusTimer, this, &UURGInteractComponent::UpdateInteractFocus, InteractFocusInterval, true);
}

void UURGInteractComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InteractFocusTimer);
	}

	Super::EndPlay(EndPlayReason);
}

bool UURGInteractComponent::TryInteract()
{
	AActor* Target = FindInteractTarget();
	if (Target == nullptr)
	{
		return false;
	}

	IURGInteractable::Execute_Interact(Target, OwnerPawn);
	return true;
}

void UURGInteractComponent::StopFocus()
{
	GetWorld()->GetTimerManager().ClearTimer(InteractFocusTimer);
	SetInteractFocus(nullptr);
}

AActor* UURGInteractComponent::FindInteractTarget()
{
	UWorld* World = GetWorld();
	if (World == nullptr || OwnerPawn == nullptr)
	{
		return nullptr;
	}

	// 카메라 시점에서 전방으로 InteractTrace. 카메라는 소유자 뒤에 있으므로 그 거리만큼 사거리를 보정한다.
	FVector ViewLocation = OwnerPawn->GetActorLocation();
	FRotator ViewRotation = OwnerPawn->GetActorRotation();
	if (AController* Controller = OwnerPawn->GetController())
	{
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}

	const float CameraOffset = FVector::Dist(ViewLocation, OwnerPawn->GetActorLocation());
	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + ViewRotation.Vector() * (InteractRange + CameraOffset);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(Interact), false);
	Params.AddIgnoredActor(OwnerPawn);

	if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_InteractTrace, Params))
	{
		return nullptr;
	}

	AActor* HitActor = Hit.GetActor();
	if (HitActor == nullptr || HitActor->Implements<UURGInteractable>() == false)
	{
		return nullptr;
	}

	return IURGInteractable::Execute_CanInteract(HitActor, OwnerPawn) ? HitActor : nullptr;
}

void UURGInteractComponent::UpdateInteractFocus()
{
	SetInteractFocus(FindInteractTarget());
}

void UURGInteractComponent::SetInteractFocus(AActor* InActor)
{
	// 같은 대상을 계속 보고 있으면 알리지 않는다 (HUD가 갱신 간격마다 다시 그리지 않도록)
	if (InteractFocus.Get() == InActor)
	{
		return;
	}

	InteractFocus = InActor;
	OnInteractFocusChanged.Broadcast(InActor);
}

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "URGMeleeTraceNotifyState.generated.h"

/**
 * 근접 판정 구간 노티파이.
 * 몽타주에서 이 구간 동안 소유 캐릭터의 주 무기(근접)에 BeginTrace/UpdateTrace/EndTrace를 전달한다.
 * 주 무기가 근접 무기가 아니면 아무것도 하지 않는다.
 */
UCLASS(meta = (DisplayName = "URG Melee Trace"))
class UNREAL_ROGUELIKE_API UURGMeleeTraceNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

private:
	/** 메시 소유 액터의 주 무기를 근접 무기로 얻는다 (없으면 nullptr) */
	class AURGMeleeWeapon* GetMeleeWeapon(const USkeletalMeshComponent* InMeshComp) const;

};


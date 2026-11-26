#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "URGMeleeTraceNotifyState.generated.h"

/**
 * 근접 판정 구간 노티파이.
 * 몽타주에서 이 구간의 시작과 끝에 소유 캐릭터의 주 무기(근접)에 BeginTrace/EndTrace를 전달한다.
 * 구간 동안의 매 프레임 판정은 무기가 자기 Tick에서 한다. 노티파이 객체는 애셋 하나를 모든 메시가 같이 써서
 * 메시별 무기를 멤버에 캐싱할 수 없으므로, 무기 찾기를 구간당 두 번으로 줄이기 위해서다.
 * 주 무기가 근접 무기가 아니면 아무것도 하지 않는다.
 */
UCLASS(meta = (DisplayName = "URG Melee Trace"))
class UNREAL_ROGUELIKE_API UURGMeleeTraceNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

private:
	/** 메시 소유 액터의 주 무기를 근접 무기로 얻는다 (없으면 nullptr) */
	class AURGMeleeWeapon* GetMeleeWeapon(const USkeletalMeshComponent* InMeshComp) const;

};


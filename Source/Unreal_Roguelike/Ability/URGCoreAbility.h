#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "URGCoreAbility.generated.h"

/**
 * 코어 부품이 제공하는 액티브 능력(Q)의 베이스.
 * 쿨다운·지속 시간·타이머와 공통 피드백(발동 메시지·파티클)을 처리하고, 실제 효과는 파생 클래스가 Activate/Deactivate에서 구현한다.
 * 파티클·메시지 같은 값은 BP 자식 클래스에서 지정한다.
 * 캐릭터가 코어 슬롯 변경 시 인스턴스를 만들고 Initialize로 소유자를 넘긴다.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class UNREAL_ROGUELIKE_API UURGCoreAbility : public UObject
{
	GENERATED_BODY()

public:
	/** UObject는 월드가 없어 타이머를 못 쓰므로 소유자의 월드를 돌려준다 */
	virtual UWorld* GetWorld() const override;

	/** 소유 캐릭터 지정 (NewObject 직후 한 번) */
	void Initialize(class AURGPlayerCharacter* InOwner);

	/** 쿨다운이 끝났으면 발동. 지속 능력이면 Duration 뒤 Deactivate */
	bool TryActivate();

	/** 발동 중이면 즉시 종료하고 타이머 해제 (사망·부품 교체용) */
	void Cancel();

	/** 쿨다운 배율 (손상 단계용, 1보다 크면 더 오래 기다림) */
	FORCEINLINE void SetCooldownScale(float InScale) { CooldownScale = FMath::Max(InScale, 0.1f); }
	FORCEINLINE bool IsActive() const { return bIsActive; }

	/** 쿨다운이 끝났는지 */
	bool IsReady() const;

protected:
	/** 효과 시작 */
	virtual void Activate() PURE_VIRTUAL(UURGCoreAbility::Activate, );

	/** 효과 종료 (Duration이 0이면 호출되지 않는다) */
	virtual void Deactivate() {}

	/** 지속 타이머에 걸 월드 초. 월드 시간 배율을 바꾸는 능력은 여기서 보정한다 */
	virtual float GetTimerDuration() const { return Duration; }

	FORCEINLINE class AURGPlayerCharacter* GetOwnerCharacter() const { return OwnerCharacter.Get(); }

private:
	/** 지속 시간 종료 (타이머 콜백) */
	void HandleDurationEnd();

	/** 지금의 지속·쿨다운 상태를 캐릭터를 거쳐 HUD에 알린다 */
	void NotifyStateChanged() const;

protected:
	/** 재사용 대기 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "CoreAbility|Rule", meta = (ClampMin = "0"))
	float Cooldown = 10.0f;

	/** 지속 시간 (초). 0이면 즉발 */
	UPROPERTY(EditDefaultsOnly, Category = "CoreAbility|Rule", meta = (ClampMin = "0"))
	float Duration = 0.0f;

	/** 발동 시 HUD 통신 로그에 띄울 문구 (비어 있으면 생략) */
	UPROPERTY(EditDefaultsOnly, Category = "CoreAbility|Feedback")
	FText ActivateMessage;

	/** 발동 시 소유자에 붙여 재생할 파티클 (없으면 생략) */
	UPROPERTY(EditDefaultsOnly, Category = "CoreAbility|Feedback")
	TObjectPtr<class UParticleSystem> ActivateEffect;

private:
	TWeakObjectPtr<class AURGPlayerCharacter> OwnerCharacter;

	FTimerHandle DurationTimer;

	/** 마지막 발동 시각 (월드 초). 음수면 아직 발동 안 함 */
	float LastActivateTime = -1.0f;

	float CooldownScale = 1.0f;

	bool bIsActive = false;

};

#pragma once

#include "CoreMinimal.h"
#include "Ability/URGCoreAbility.h"
#include "URGTimeSlowAbility.generated.h"

/**
 * 시간 감속: 지속 시간 동안 월드 시간 배율을 낮추고 소유자만 정상 속도로 움직인다.
 * 월드 타이머는 전역 배율을 타므로 지속 타이머를 배율만큼 줄여 실시간 Duration초가 되게 한다.
 * 지속 중 소유자 카메라의 후처리(채도·틴트·비네팅)를 덮어써 화면으로 감속을 알린다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGTimeSlowAbility : public UURGCoreAbility
{
	GENERATED_BODY()

public:
	UURGTimeSlowAbility();

protected:
	virtual void Activate() override;
	virtual void Deactivate() override;
	virtual float GetTimerDuration() const override;

protected:
	/** 감속 중 월드 시간 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "CoreAbility|TimeSlow", meta = (ClampMin = "0.05", ClampMax = "1"))
	float TimeDilation = 0.3f;

	/** 감속 중 화면 채도 (0 = 흑백) */
	UPROPERTY(EditDefaultsOnly, Category = "CoreAbility|TimeSlow|Screen", meta = (ClampMin = "0", ClampMax = "1"))
	float ScreenSaturation = 0.3f;

	/** 감속 중 화면 색조 */
	UPROPERTY(EditDefaultsOnly, Category = "CoreAbility|TimeSlow|Screen")
	FLinearColor ScreenTint = FLinearColor(0.7f, 0.85f, 1.2f, 1.0f);

	/** 감속 중 비네팅 강도 */
	UPROPERTY(EditDefaultsOnly, Category = "CoreAbility|TimeSlow|Screen", meta = (ClampMin = "0", ClampMax = "1"))
	float ScreenVignette = 0.6f;

};

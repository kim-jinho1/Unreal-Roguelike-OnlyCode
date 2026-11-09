#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "URGRadarComponent.generated.h"

/** 레이더 탐지 결과 알림 (탐지된 적들의 월드 위치. 꺼지면 빈 배열) */
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateOnRadarUpdated, const TArray<FVector>&);

/**
 * 적 탐지 레이더 (머리 부품 "적 탐지 레이더").
 * 켜져 있는 동안 일정 간격으로 주변의 살아 있는 캐릭터를 찾아 위치를 OnRadarUpdated로 알린다. 표시는 HUD가 한다.
 * 적 클래스를 참조하지 않고 AURGCharacter 베이스로만 거른다. 켜고 끄는 것은 소유 캐릭터가 머리 부품 반영 시 정한다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_ROGUELIKE_API UURGRadarComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UURGRadarComponent();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	FORCEINLINE bool IsRadarEnabled() const { return bRadarEnabled; }

	/** 레이더 켜기/끄기. 끄면 빈 목록을 한 번 알려 HUD 마커를 지운다 */
	void SetRadarEnabled(bool bInEnabled);

public:
	/** 탐지할 때마다 호출 */
	FDelegateOnRadarUpdated OnRadarUpdated;

protected:
	/** 탐지 반경 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radar|Scan", meta = (ClampMin = "0"))
	float RadarRange = 3000.0f;

	/** 탐지 간격 (초). 매 프레임 대신 이 간격으로만 찾는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radar|Scan", meta = (ClampMin = "0.05"))
	float ScanInterval = 0.2f;

	/** 레이더 동작 중 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radar|Scan")
	bool bRadarEnabled = false;

private:
	/** 타이머 콜백: 반경 안의 살아 있는 다른 캐릭터를 찾아 알린다 */
	void Scan();

private:
	FTimerHandle ScanTimer;

};


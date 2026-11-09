#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "URGWeaponCameraShake.generated.h"

/**
 * 무기 발사·근접 적중 시 짧게 흔드는 카메라 셰이크.
 * 펄린 노이즈 패턴을 C++ 기본값으로 정의해 애셋 없이 쓴다. 세기는 재생할 때 무기마다 배율(Scale)로 조절한다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGWeaponCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UURGWeaponCameraShake(const FObjectInitializer& ObjectInitializer);

};


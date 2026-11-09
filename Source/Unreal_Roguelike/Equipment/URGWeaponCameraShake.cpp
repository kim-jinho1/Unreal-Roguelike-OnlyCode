#include "Equipment/URGWeaponCameraShake.h"
#include "Shakes/PerlinNoiseCameraShakePattern.h"

UURGWeaponCameraShake::UURGWeaponCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPerlinNoiseCameraShakePattern>(TEXT("RootShakePattern")))
{
	// 연사 중에도 한 발마다 새로 흔들리도록 겹쳐 재생한다
	bSingleInstance = false;

	UPerlinNoiseCameraShakePattern* Pattern = Cast<UPerlinNoiseCameraShakePattern>(GetRootShakePattern());
	if (Pattern == nullptr)
	{
		return;
	}

	// 짧고 날카롭게: 바로 최대 세기로 시작해 빠르게 잦아든다
	Pattern->Duration = 0.18f;
	Pattern->BlendInTime = 0.0f;
	Pattern->BlendOutTime = 0.12f;

	// 위치는 살짝만, 회전(특히 피치)을 주로 흔든다. FOV는 건드리지 않는다
	Pattern->X.Amplitude = 0.0f;
	Pattern->Y.Amplitude = 1.0f;
	Pattern->Y.Frequency = 30.0f;
	Pattern->Z.Amplitude = 1.0f;
	Pattern->Z.Frequency = 30.0f;

	Pattern->Pitch.Amplitude = 1.0f;
	Pattern->Pitch.Frequency = 25.0f;
	Pattern->Yaw.Amplitude = 0.6f;
	Pattern->Yaw.Frequency = 25.0f;
	Pattern->Roll.Amplitude = 0.4f;
	Pattern->Roll.Frequency = 20.0f;

	Pattern->FOV.Amplitude = 0.0f;
}


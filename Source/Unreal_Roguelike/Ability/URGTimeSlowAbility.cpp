#include "Ability/URGTimeSlowAbility.h"
#include "Camera/CameraComponent.h"
#include "Characters/URGPlayerCharacter.h"
#include "Engine/Scene.h"
#include "Kismet/GameplayStatics.h"

UURGTimeSlowAbility::UURGTimeSlowAbility()
{
	Duration = 4.0f;
}

void UURGTimeSlowAbility::Activate()
{
	AURGPlayerCharacter* Owner = GetOwnerCharacter();
	if (Owner == nullptr)
	{
		return;
	}

	UGameplayStatics::SetGlobalTimeDilation(Owner, TimeDilation);

	// 전역 배율을 자기 배율로 상쇄해 소유자만 정상 속도
	Owner->CustomTimeDilation = 1.0f / TimeDilation;

	// 카메라 후처리 덮어쓰기: 채도를 빼고 푸른 틴트 + 비네팅으로 감속을 보여 준다
	if (UCameraComponent* Camera = Owner->GetCamera())
	{
		FPostProcessSettings& Settings = Camera->PostProcessSettings;
		Settings.bOverride_ColorSaturation = true;
		Settings.ColorSaturation = FVector4(ScreenSaturation, ScreenSaturation, ScreenSaturation, 1.0f);
		Settings.bOverride_SceneColorTint = true;
		Settings.SceneColorTint = ScreenTint;
		Settings.bOverride_VignetteIntensity = true;
		Settings.VignetteIntensity = ScreenVignette;
	}
}

void UURGTimeSlowAbility::Deactivate()
{
	AURGPlayerCharacter* Owner = GetOwnerCharacter();
	if (Owner == nullptr)
	{
		return;
	}

	UGameplayStatics::SetGlobalTimeDilation(Owner, 1.0f);
	Owner->CustomTimeDilation = 1.0f;

	if (UCameraComponent* Camera = Owner->GetCamera())
	{
		FPostProcessSettings& Settings = Camera->PostProcessSettings;
		Settings.bOverride_ColorSaturation = false;
		Settings.bOverride_SceneColorTint = false;
		Settings.bOverride_VignetteIntensity = false;
	}
}

float UURGTimeSlowAbility::GetTimerDuration() const
{
	// 월드 타이머는 감속된 시간으로 흐르므로 그만큼 줄여야 실시간 Duration초가 된다
	return Duration * TimeDilation;
}

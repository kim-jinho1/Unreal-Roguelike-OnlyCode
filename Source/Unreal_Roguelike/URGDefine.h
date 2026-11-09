#pragma once

#include "CoreMinimal.h"
#include "URGDefine.generated.h"

/** 프로젝트 커스텀 트레이스 채널 (DefaultEngine.ini의 GameTraceChannel 순서와 일치) */
#define ECC_WeaponTrace ECC_GameTraceChannel1
#define ECC_InteractTrace ECC_GameTraceChannel2

/** 부품 슬롯 (기획서: 팔 = 주 공격, 다리 = 이동/회피, 코어 = 액티브, 머리 = 패시브) */
UENUM(BlueprintType)
enum class EURGPartSlot : uint8
{
	Arm,
	Leg,
	Core,
	Head,
};

/** 부품 손상 단계 (기획서: 정상 → 손상 → 파손). HUD 슬롯 테두리 색 = 흰 / 노랑 / 빨강 */
UENUM(BlueprintType)
enum class EURGPartCondition : uint8
{
	Normal,
	Damaged,
	Broken,
};

/** 피격 방향 (피격 리액션 애님 선택용) */
UENUM(BlueprintType)
enum class EURGHitDirection : uint8
{
	Front,
	Back,
	Left,
	Right,
};

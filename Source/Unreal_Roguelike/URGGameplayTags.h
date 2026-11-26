#pragma once

#include "NativeGameplayTags.h"

/**
 * 프로젝트 전역 네이티브 게임플레이 태그.
 * 문자열 대신 이 심볼을 사용해 오타를 컴파일 시점에 잡는다.
 */
namespace URGGameplayTags
{
	/** 캐릭터 상태 (StateComponent가 하나만 보유) */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Dashing);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_GeneralAction);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Hit);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Death);

	/** 부품 시너지 태그 (기획서: 동일 태그 2개 장착 시 효과 발동) */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Tag_Thermal);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Tag_Electric);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Part_Tag_Reinforce);
}

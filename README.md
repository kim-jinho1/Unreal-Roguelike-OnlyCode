# 언리얼 로그라이크 (Unreal Roguelike)

부품 하나로 싸우는 방식이 바뀌는, 정비 로봇의 3인칭 슈팅 로그라이크입니다.
2026년도 2학기 언리얼 팀 프로젝트이며, 이 저장소에는 팀에서 **제가 맡은 플레이어 · 전투 쪽 코드만** 모았습니다.

- **포트폴리오**: [언리얼 로그라이크 (노션)](https://www.notion.so/3f2048b7ae3481909bb3c6f12efe70dd)
- **플레이 영상**: https://youtu.be/0J_BJj9XG8w
- **인원 · 기간**: 2명 (프로그래머 2명) · 2026.08.19 ~ 2026.10.07 · 저는 팀장 · 클라이언트 프로그래밍
- **엔진**: Unreal Engine 5.8 · C++

> 이 저장소는 **코드 열람용**입니다. 팀 저장소는 비공개이고, 적 AI · 스테이지 흐름 · 보스 · 일부 미니게임 등 팀원 코드와
> 에셋(Paragon 등)이 빠져 있어서 이 저장소만으로는 빌드되지 않습니다.

## 먼저 볼 코드

기준 폴더는 `Source/Unreal_Roguelike/`입니다.

- **캐릭터 (중재자)** — 입력 → 차단 검사 → 컴포넌트 호출, 부품 효과 반영, 피격 · 사망 처리
  - `Characters/URGPlayerCharacter.h`
- **부품 데이터** — 슬롯 · 이름 · 아이콘 · 시너지 태그 · 내구도를 한 에셋에 담습니다
  - `Equipment/URGPartDataAsset.h`
- **부품 상태** — 슬롯 4개의 장착 부품 · 내구도 · 손상 단계 · 켜진 시너지를 들고, `OnSlotChanged` · `OnSynergyChanged`로 알립니다. 맞은 뼈로 부위를 찾는 표도 여기 있습니다
  - `Component/URGPartComponent.h`
- **코어 능력** — Q 능력 인스턴스를 들고 교체 · 발동 · 받는 피해 배율(실드)을 맡습니다. 능력 자체는 쿨다운 · 지속 시간만 처리하는 작은 `UObject` 베이스에서 파생합니다
  - `Component/URGCoreAbilityComponent.h` · `Ability/URGCoreAbility.h`
- **전투** — 무기 소유(스폰 · 제거는 여기서만), 공격 간격, 재장전, 적중 시 시너지 효과
  - `Component/URGCombatComponent.h` · `Equipment/URGWeapon.h`
- **수리** — 슬롯별 규칙 표(미니게임 클래스 + 기준 점수)로 미니게임을 열고, 점수(0~1)가 기준 이상인지만 봅니다
  - `Equipment/URGRepairStation.h` · `UI/URGMinigameUI.h`
- **HUD** — 컴포넌트 델리게이트를 구독하고 `NativeDestruct`에서 해제합니다. 히트 마커 · 데미지 숫자는 별도 위젯입니다
  - `UI/URGPlayerHUD.h` · `UI/URGHitFeedbackUI.h`

**리팩토링 (2026.11)**: 발표 뒤 플레이어 캐릭터에 몰려 있던 대시 · 상호작용 · 재장전 · Q 능력 · 시너지 기록 · 피격 연출을 컴포넌트로 나누고, 캐릭터는 중재만 하게 바꿨습니다.
`URGPlayerCharacter.h/.cpp`는 1,496줄에서 978줄이 됐고, 그 과정에서 근접 판정 · 재장전 취소 · 코어 손상 시 HUD 쿨다운 어긋남 · 수리 비용 순서 버그도 고쳤습니다.

## 출처

작성 주체별 구분입니다.

| 부분 | 출처 |
|---|---|
| `URGStateComponent` · `URGAttributeComponent` · `URGAimComponent` · `URGAnimInstance` | 참고 프로젝트의 코드를 가져와 수정 |
| `URGPlayerHUD` · `URGRepairStation` · `URGPartSlotUI` · `URGBullet` · `URGPlayerCharacter.cpp` | 팀원이 연결용으로 덧붙인 코드가 일부 섞여 있음 |
| 시너지 효과 · 부품 비교 UI · HUD 개편 · 타이틀 · 엔딩 등 (10/6 이후 추가) | AI 코딩 도구(Claude Code)를 활용해 구현 |
| 2026.11 리팩토링 | 무엇을 나눌지는 제가 정하고, 구현은 AI 코딩 도구(Claude Code)가 하고, 제가 검수 · 테스트 · 커밋 |

## 의존성 (이 저장소에 포함되지 않음)

Unreal Engine 5.8 · Paragon 에셋 (팀 저장소)

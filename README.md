# Unreal Roguelike — 플레이어 · 전투 코드

부품 하나로 싸우는 방식이 바뀌는, 정비 로봇의 3인칭 슈팅 로그라이크입니다.
팀 프로젝트 중 **제가 맡은 플레이어 · 전투 쪽 코드만** 모은 열람용 저장소입니다.

- **포트폴리오**: [언리얼 로그라이크 (노션)](https://www.notion.so/3f2048b7ae3481909bb3c6f12efe70dd)
- **플레이 영상**: https://youtu.be/s9Lx9X6Hd7w
- **인원 · 기간**: 2명 (프로그래머 2명) · 2026.08.19 ~ 2026.10.07 · 저는 팀장 · 클라이언트 프로그래밍
- **엔진**: Unreal Engine 5.8 · C++

> 이 저장소는 **코드 열람용**입니다. 팀 저장소는 비공개이고, 적 AI · 스테이지 흐름 · 보스 · 일부 미니게임 등 팀원 코드와
> 에셋(Paragon 등)이 빠져 있어서 이 저장소만으로는 빌드되지 않습니다.

## 먼저 볼 코드

기준 폴더는 `Source/Unreal_Roguelike/`입니다.

- **부품 데이터** — 슬롯 · 이름 · 아이콘 · 시너지 태그 · 내구도를 한 에셋에 담습니다
  - `Equipment/URGPartDataAsset.h`
- **부품 상태** — 슬롯 4개의 장착 부품 · 내구도 · 손상 단계만 들고, 바뀔 때마다 `OnSlotChanged`로 알립니다
  - `Component/URGPartComponent.h`
- **코어 능력** — 쿨다운 · 지속 시간만 처리하는 작은 `UObject` 베이스입니다
  - `Ability/URGCoreAbility.h`
- **수리** — 수리 장치는 미니게임의 점수(0~1)만 받아 기준을 넘는지 봅니다
  - `Equipment/URGRepairStation.h` · `UI/URGMinigameUI.h`
- **상호작용** — 상호작용 대상이 구현하는 인터페이스입니다
  - `Interface/URGInteractable.h`
- **구성 요소** — 캐릭터에 입력 · 조준 · 전투 · 부품 · 자원 · 레이더 컴포넌트를 붙이는 구조입니다
  - `Characters/` · `Component/` · `Equipment/` · `UI/`

## 출처

작성 주체별 구분입니다.

| 부분 | 출처 |
|---|---|
| `URGStateComponent` · `URGAttributeComponent` · `URGAimComponent` · `URGAnimInstance` | 참고 프로젝트의 코드를 가져와 수정 |
| `URGPlayerHUD` · `URGRepairStation` · `URGPartSlotUI` · `URGBullet` · `URGPlayerCharacter.cpp` | 팀원이 연결용으로 덧붙인 코드가 일부 섞여 있음 |
| 시너지 효과 · 부품 비교 UI · HUD 개편 · 타이틀 · 엔딩 등 (10/6 이후 추가) | AI 코딩 도구(Claude Code)를 활용해 구현 |

## 의존성 (이 저장소에 포함되지 않음)

Unreal Engine 5.8 · Paragon 에셋 (팀 저장소)

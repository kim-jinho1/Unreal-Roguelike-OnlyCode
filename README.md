# Unreal Roguelike — 플레이어 · 전투 코드

UE 5.8 C++ 3인칭 슈팅 로그라이크 (2인 팀 프로젝트, 2026.08.19 ~ 10.07) 중 **제가 맡은 플레이어 · 전투 쪽 코드만** 모은 열람용 저장소입니다.

- 포트폴리오: [언리얼 로그라이크 (노션)](https://www.notion.so/3f2048b7ae3481909bb3c6f12efe70dd)
- 팀 저장소는 비공개입니다. 적 AI · 스테이지 흐름 · 보스 · 일부 미니게임 등 팀원 코드와 에셋(Paragon 등)이 빠져 있어 **이 저장소만으로는 빌드되지 않습니다.**
- `URGPlayerHUD`, `URGRepairStation`, `URGPartSlotUI`, `URGBullet`, `URGPlayerCharacter.cpp`에는 팀원이 연결용으로 덧붙인 코드가 일부 섞여 있습니다.
- `URGStateComponent` · `URGAttributeComponent` · `URGAimComponent` · `URGAnimInstance`는 참고 프로젝트의 코드를 가져와 수정했습니다.
- 10/6 이후 추가된 기능(시너지 효과, 부품 비교 UI, HUD 개편, 타이틀 · 엔딩 등)은 AI 코딩 도구(Claude Code)를 활용해 구현했습니다.

| 폴더 (`Source/Unreal_Roguelike/`) | 내용 |
|---|---|
| `Characters/` | 공용 베이스 `AURGCharacter`, `AURGPlayerCharacter` |
| `Component/` | 입력 · 상태 · 체력 · 조준 · 전투 · 부품 · 자원 · 레이더 · 화상 |
| `Ability/` | 코어 부품 액티브 능력 (폭발 · 실드 · 시간 감속) |
| `Equipment/` | 무기 계층, 부품 데이터 에셋, 픽업, 수리 장치 |
| `Animation/` | 애님 인스턴스, 근접 판정 노티파이 |
| `UI/` | HUD와 하위 위젯, 부품 비교 · 수리 · 게임오버 · 타이틀 |
| `Core/` | 게임 모드, 플레이어 컨트롤러 |

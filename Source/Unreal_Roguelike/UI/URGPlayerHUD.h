#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "URGDefine.h"
#include "URGPlayerHUD.generated.h"

struct FURGPartSlotState;
struct FGameplayTag;
struct FGameplayTagContainer;

/** 화면에 떠 있는 데미지 숫자 하나 */
struct FURGDamageNumber
{
	/** 맞은 월드 위치 (매 페인트마다 화면 좌표로 투영한다) */
	FVector WorldLocation = FVector::ZeroVector;

	/** 숫자가 겹치지 않도록 생성 시 정한 화면 오프셋 */
	FVector2D ScreenJitter = FVector2D::ZeroVector;

	FText Text;

	/** 생성 시각 (실제 초) */
	float SpawnTime = 0.0f;

	bool bCrit = false;
};

/** 슬롯마다 마지막으로 본 부품과 손상 단계 (같은 부품이 나빠졌을 때만 알리기 위한 기록) */
struct FURGSlotConditionRecord
{
	TWeakObjectPtr<const class UURGPartDataAsset> Part;

	EURGPartCondition Condition = EURGPartCondition::Normal;
};

/**
 * 플레이어 HUD 루트 위젯.
 * 폰의 Attribute/Resource/Part/Combat 컴포넌트 델리게이트를 구독해 하위 위젯에 값을 전달한다.
 * 스테이지 정보와 관제 통신 텍스트는 던전/이벤트 시스템이 BlueprintCallable 함수로 직접 밀어 넣는다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGPlayerHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 플레이어 폰의 컴포넌트에 바인딩하고 현재 값을 즉시 반영한다.
	 * 위젯 생성이 OnPossess보다 앞설 수 있으므로 폰을 확보한 AURGPlayerController가 명시적으로 호출한다.
	 */
	void InitializeForPawn(APawn* InPawn);

	/** 관제 AI 통신 텍스트 표시 (InDuration 초 후 자동 숨김) */
	UFUNCTION(BlueprintCallable, Category = "PlayerHUD|Comm")
	void ShowCommMessage(const FText& InMessage, float InDuration = 3.0f);

	/** 코어 능력 발동 (플레이어 캐릭터 OnCoreAbilityActivated): 문구가 있으면 통신 로그에 표시 */
	void HandleCoreAbilityActivated(const FText& InMessage);

	/** 코어 능력 상태 변경 (플레이어 캐릭터 OnCoreAbilityStateChanged): 코어 슬롯에 지속·쿨다운 표시 */
	void HandleCoreAbilityStateChanged(bool bInActive, float InCooldownEndTime, float InCooldownDuration);

	/** 대시 쿨다운 변경 (플레이어 캐릭터 OnDashCooldownChanged): 다리 슬롯에 쿨다운 표시 */
	void HandleDashCooldownChanged(float InCooldownEndTime, float InCooldownDuration);

	/** 공격 쿨다운 변경 (플레이어 캐릭터 OnAttackCooldownChanged): 팔 슬롯에 쿨다운 표시 */
	void HandleAttackCooldownChanged(float InCooldownEndTime, float InCooldownDuration);

	/** 스테이지·방 진행 텍스트 갱신 (던전 시스템이 방 전환 시 호출) */
	UFUNCTION(BlueprintCallable, Category = "PlayerHUD|Stage")
	void SetStageInfo(int32 InStage, int32 InRoom, int32 InTotalRooms);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	void HandleHealthChanged(float InCurrent, float InMax) const;
	void HandleResourceChanged(int32 InResource) const;
	void HandleSlotChanged(EURGPartSlot InSlot, const FURGPartSlotState& InState);
	void HandleAmmoChanged(int32 InCurrent, int32 InMagazineSize) const;

	/** 무기 교체 알림을 받아 새 무기의 탄약 델리게이트로 구독을 옮긴다 */
	void HandleWeaponChanged();

	/** 시너지 발동·해제 (플레이어 캐릭터 OnSynergyChanged): 통신 로그 문구 + 네 슬롯의 태그 색 갱신 */
	void HandleSynergyChanged(const FGameplayTag& InTag, bool bInActive);

	/** 바라보는 상호작용 대상 변경 (플레이어 캐릭터 OnInteractFocusChanged): 부품 픽업이면 비교 UI를 띄운다 */
	void HandleInteractFocusChanged(AActor* InActor);

	/** 레이더 탐지 결과 (RadarComponent OnRadarUpdated): 적 방향 마커 갱신 */
	void HandleRadarUpdated(const TArray<FVector>& InEnemyLocations);

	/** 무기 적중 (CombatComponent OnHitDealt): 히트 마커를 켜고 데미지 숫자를 추가한다 */
	void HandleHitDealt(const FVector& InLocation, float InDamage, bool bInCrit, bool bInKilled);

private:
	/** 히트 마커(크로스헤어 둘레 X)와 데미지 숫자를 그린다. 표시 중인 것이 없으면 아무것도 그리지 않는다 */
	int32 PaintHitFeedback(const FGeometry& InGeometry, FSlateWindowElementList& OutElements, int32 InLayerId) const;

	/** 피드백 애니메이션 기준 시각. 히트스톱·시간 감속에 영향받지 않도록 실제 초 */
	float GetFeedbackTime() const;

	friend class FURGBossHUDLifecycleTest;
	/** 늦게 스폰되는 보스도 찾되, 체력 갱신은 컴포넌트 이벤트로 처리한다. */
	void RefreshBossTarget();
	void BindBoss(class AURGTempleBoss* InBoss);
	void ClearBoss();
	void HandleBossHealthChanged(float InCurrent, float InMax);
	void HandleBossDeath();
	/** 슬롯 부품의 시너지 태그 이름을 모아 표시. 부품이 쓸 수 있고 태그 중 하나라도 발동 중이면 강조 */
	void RefreshSlotSynergyTags(EURGPartSlot InSlot, const FURGPartSlotState& InState) const;

	/** 시너지 태그의 표시 이름 (모르는 태그면 빈 텍스트) */
	const FText& GetSynergyTagName(const FGameplayTag& InTag) const;

	/** 시너지 태그의 발동 색 (모르는 태그면 미발동 색) */
	const FLinearColor& GetSynergyTagColor(const FGameplayTag& InTag) const;

	/** 모은 개수만큼 채운 점, 남은 만큼 빈 점 ("●○") */
	FText BuildSynergyPips(int32 InCount, int32 InThreshold) const;

	/** 같은 부품이 손상·파손으로 나빠졌으면 통신 로그로 알린다. 첫 반영·교체·수리는 알리지 않는다 */
	void NotifyPartConditionChange(EURGPartSlot InSlot, const FURGPartSlotState& InState);

	/** 시너지 효과 설명. 숫자는 캐릭터·CombatComponent의 현재 값에서 가져온다 (밸런싱 값과 문구가 어긋나지 않도록) */
	FText BuildSynergyDescription(const FGameplayTag& InTag) const;

	/** 태그마다 한 줄: 발동 중이면 "이름 ●● 효과"(태그 색), 모으는 중이면 "이름 ●○"(회색), 하나도 없으면 숨김 */
	void RefreshSynergyPanel() const;

	/** 태그 컨테이너의 시너지 태그 이름을 구분자로 이은 문구 (모르는 태그는 건너뜀) */
	FText BuildTagNamesText(const FGameplayTagContainer& InTags) const;

	/** 바라보는 픽업이 있으면 현재 부품과 비교해 비교 UI를 채우고, 없으면 숨긴다 */
	void RefreshPartCompare() const;

	/** 0 나눗셈을 피한 비율(0.0 ~ 1.0) 계산 */
	float GetSafeRatio(float InCurrent, float InMax) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Boss")
	FText BossDisplayName = NSLOCTEXT("URG.PlayerHUD", "BossName", "보스");

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UURGHealthBarUI> HealthBarUI;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UURGResourceUI> ResourceUI;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UURGAmmoUI> AmmoUI;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UURGPartSlotsUI> PartSlotsUI;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UURGStageInfoUI> StageInfoUI;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UURGCommLogUI> CommLogUI;

	/** 발동 중인 시너지 목록. 없는 WBP도 동작하도록 선택 바인딩 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UURGSynergyPanelUI> SynergyPanelUI;

	/** 부품 비교 창. 선택 바인딩 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UURGPartCompareUI> PartCompareUI;

	/** 적 방향 마커 (레이더 머리 부품). 선택 바인딩 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UURGEnemyRadarUI> EnemyRadarUI;

	/** 열 시너지 태그 표시 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FText ThermalTagName = NSLOCTEXT("URG.PlayerHUD", "ThermalTag", "열");

	/** 전기 시너지 태그 표시 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FText ElectricTagName = NSLOCTEXT("URG.PlayerHUD", "ElectricTag", "전기");

	/** 강화 시너지 태그 표시 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FText ReinforceTagName = NSLOCTEXT("URG.PlayerHUD", "ReinforceTag", "강화");

	/** 시너지 발동 시 통신 로그 문구 ({0} = 태그 이름, {1} = 효과 설명) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FText SynergyOnFormat = NSLOCTEXT("URG.PlayerHUD", "SynergyOn", "{0} 시너지 활성 — {1}");

	/** 시너지 패널: 발동 중인 줄 ({0} = 태그 이름, {1} = 진행 점, {2} = 효과 설명) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FText SynergyActiveRowFormat = NSLOCTEXT("URG.PlayerHUD", "SynergyActiveRow", "{0} {1}  {2}");

	/** 시너지 패널: 모으는 중인 줄 ({0} = 태그 이름, {1} = 진행 점) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FText SynergyProgressRowFormat = NSLOCTEXT("URG.PlayerHUD", "SynergyProgressRow", "{0} {1}");

	/** 진행 점: 모은 칸 */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FText SynergyFilledPip = NSLOCTEXT("URG.PlayerHUD", "SynergyFilledPip", "●");

	/** 진행 점: 빈 칸 */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FText SynergyEmptyPip = NSLOCTEXT("URG.PlayerHUD", "SynergyEmptyPip", "○");

	/** 부품 슬롯의 시너지 태그 표시 ({0} = 태그 이름들). 부품 이름과 구분되도록 괄호로 묶는다 */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FText SlotTagFormat = NSLOCTEXT("URG.PlayerHUD", "SlotTag", "[{0}]");

	/** 강화 시너지 색 (발동 중 글자) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FLinearColor ReinforceTagColor = FLinearColor(0.7f, 0.55f, 1.0f, 1.0f);

	/** 열 시너지 색 (발동 중 글자) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FLinearColor ThermalTagColor = FLinearColor(1.0f, 0.4f, 0.1f, 1.0f);

	/** 전기 시너지 색 (발동 중 글자) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FLinearColor ElectricTagColor = FLinearColor(0.3f, 0.85f, 1.0f, 1.0f);

	/** 발동 전 시너지 글자 색 */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FLinearColor SynergyInactiveColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	/** 열 시너지 효과 ({0} = 지속 초, {1} = 간격 초, {2} = 한 번 피해) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FText ThermalDescFormat = NSLOCTEXT("URG.PlayerHUD", "ThermalDesc", "적중 시 {0}초간 화상 ({1}초마다 {2})");

	/** 전기 시너지 효과 ({0} = 반경 m, {1} = 최대 대상 수, {2} = 전이 비율 %) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FText ElectricDescFormat = NSLOCTEXT("URG.PlayerHUD", "ElectricDesc", "적중 시 {0}m 안 적 {1}명에게 피해 {2}% 전이");

	/** 강화 시너지 효과 ({0} = 최대 체력 증가, {1} = 부품 손상 감소 %) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FText ReinforceDescFormat = NSLOCTEXT("URG.PlayerHUD", "ReinforceDesc", "최대 체력 +{0}, 부품 손상 -{1}%");

	/** 시너지 해제 시 통신 로그 문구 ({0} = 태그 이름) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FText SynergyOffFormat = NSLOCTEXT("URG.PlayerHUD", "SynergyOff", "{0} 시너지 해제");

	/** 비교 UI: 장착하면 켜지는 시너지 ({0} = 태그 이름, {1} = 효과 설명) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FText PreviewOnFormat = NSLOCTEXT("URG.PlayerHUD", "PreviewOn", "장착 시 {0} 시너지 활성 — {1}");

	/** 비교 UI: 장착하면 꺼지는 시너지 ({0} = 태그 이름) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FText PreviewOffFormat = NSLOCTEXT("URG.PlayerHUD", "PreviewOff", "장착 시 {0} 시너지 해제");

	/** 슬롯에 태그가 여러 개일 때 사이에 넣는 구분자 */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|Synergy")
	FText SynergyTagSeparator = NSLOCTEXT("URG.PlayerHUD", "TagSeparator", " · ");

	/** 부품 손상 알림 ({0} = 부품 이름) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|PartCondition")
	FText PartDamagedFormat = NSLOCTEXT("URG.PlayerHUD", "PartDamaged", "{0} 손상 — 성능 저하");

	/** 부품 파손 알림 ({0} = 부품 이름, {1} = 슬롯별 파손 효과) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|PartCondition")
	FText PartBrokenFormat = NSLOCTEXT("URG.PlayerHUD", "PartBroken", "{0} 파손 — {1}");

	/** 슬롯별 파손 효과 문구 */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|PartCondition")
	TMap<EURGPartSlot, FText> BrokenEffectTexts = {
		{ EURGPartSlot::Arm, NSLOCTEXT("URG.PlayerHUD", "BrokenArm", "근접 공격으로 전환") },
		{ EURGPartSlot::Leg, NSLOCTEXT("URG.PlayerHUD", "BrokenLeg", "이동 강화·대시 정지") },
		{ EURGPartSlot::Core, NSLOCTEXT("URG.PlayerHUD", "BrokenCore", "코어 능력 사용 불가") },
		{ EURGPartSlot::Head, NSLOCTEXT("URG.PlayerHUD", "BrokenHead", "센서 기능 정지") },
	};

	/** 부품 손상·파손 알림 표시 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|PartCondition", meta = (ClampMin = "0.1"))
	float PartConditionMessageDuration = 4.0f;

	/** 히트 마커 표시 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|HitFeedback", meta = (ClampMin = "0"))
	float HitMarkerDuration = 0.15f;

	/** 처치 마커 표시 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|HitFeedback", meta = (ClampMin = "0"))
	float KillMarkerDuration = 0.3f;

	/** 화면 중앙에서 X 선이 시작되는 거리 (px) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|HitFeedback", meta = (ClampMin = "0"))
	float HitMarkerGap = 8.0f;

	/** X 선 한 획의 길이 (px) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|HitFeedback", meta = (ClampMin = "1"))
	float HitMarkerLength = 10.0f;

	/** 처치 마커 크기 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|HitFeedback", meta = (ClampMin = "1"))
	float KillMarkerScale = 1.4f;

	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|HitFeedback")
	FLinearColor HitMarkerColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|HitFeedback")
	FLinearColor KillMarkerColor = FLinearColor(1.0f, 0.15f, 0.1f, 1.0f);

	/** 데미지 숫자가 떠 있는 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|HitFeedback", meta = (ClampMin = "0.1"))
	float DamageNumberDuration = 0.7f;

	/** 데미지 숫자가 사라지기 전까지 떠오르는 높이 (px) */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|HitFeedback", meta = (ClampMin = "0"))
	float DamageNumberRise = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|HitFeedback", meta = (ClampMin = "6"))
	int32 DamageNumberFontSize = 18;

	/** 치명타 숫자 크기 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|HitFeedback", meta = (ClampMin = "1"))
	float CritFontScale = 1.4f;

	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|HitFeedback")
	FLinearColor DamageNumberColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "PlayerHUD|HitFeedback")
	FLinearColor CritDamageNumberColor = FLinearColor(1.0f, 0.85f, 0.1f, 1.0f);

private:
	TWeakObjectPtr<class AURGTempleBoss> BoundBoss;
	TWeakObjectPtr<class UURGAttributeComponent> BossAttribute;
	TWeakObjectPtr<class UWorld> BossTrackingWorld;
	FDelegateHandle BossHealthHandle;
	FDelegateHandle BossDeathHandle;
	FTimerHandle BossSearchTimer;
	float BossCurrentHealth = 0.0f;
	float BossMaxHealth = 0.0f;
	bool bShowBossHealth = false;
	/** 플레이어 캐릭터 (발동 중인 시너지 조회용) */
	TWeakObjectPtr<class AURGPlayerCharacter> PlayerCharacter;

	/** 지금 바라보는 부품 픽업 (주워지면 사라지므로 약참조) */
	TWeakObjectPtr<class AURGPartPickup> FocusedPickup;

	/** 현재 탄약 구독이 걸려 있는 원거리 무기 (근접 무기면 없음) */
	TWeakObjectPtr<class AURGRangedWeapon> BoundWeapon;

	/** 폰의 전투 컴포넌트 */
	TWeakObjectPtr<class UURGCombatComponent> CombatComponent;

	/** 슬롯마다 마지막으로 본 부품·손상 단계 (손상 알림 판단용) */
	TMap<EURGPartSlot, FURGSlotConditionRecord> LastSlotConditions;

	/** 떠 있는 데미지 숫자 (적중 때 추가, 오래된 것은 다음 적중 때 정리) */
	TArray<FURGDamageNumber> DamageNumbers;

	/** 마지막 적중·처치 시각 (실제 초). 처음엔 아주 과거로 두어 마커가 꺼져 있게 한다 */
	float LastHitTime = -1000.0f;
	float LastKillTime = -1000.0f;

};


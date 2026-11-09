#include "UI/URGPlayerHUD.h"
#include "Characters/URGPlayerCharacter.h"
#include "Characters/URGTempleBoss.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "Component/URGAttributeComponent.h"
#include "Component/URGCombatComponent.h"
#include "Component/URGPartComponent.h"
#include "Component/URGRadarComponent.h"
#include "Component/URGResourceComponent.h"
#include "Equipment/URGPartDataAsset.h"
#include "Equipment/URGPartPickup.h"
#include "Equipment/URGRangedWeapon.h"
#include "GameFramework/Pawn.h"
#include "GameplayTagContainer.h"
#include "UI/URGAmmoUI.h"
#include "UI/URGCommLogUI.h"
#include "UI/URGEnemyRadarUI.h"
#include "UI/URGHealthBarUI.h"
#include "UI/URGPartCompareUI.h"
#include "UI/URGPartSlotsUI.h"
#include "UI/URGResourceUI.h"
#include "UI/URGStageInfoUI.h"
#include "UI/URGSynergyPanelUI.h"
#include "URGGameplayTags.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
	/** 동시에 띄우는 데미지 숫자 상한 (미사일 다발이 한 번에 많이 맞아도 화면이 덮이지 않도록) */
	constexpr int32 MaxDamageNumbers = 24;

	/** 데미지 숫자가 이 비율만큼 지난 뒤부터 흐려진다 */
	constexpr float DamageNumberFadeStart = 0.6f;
}

void UURGPlayerHUD::NativeConstruct()
{
	Super::NativeConstruct();
	if (UWorld* World = GetWorld(); World && World->IsGameWorld())
	{
		BossTrackingWorld = World;
		RefreshBossTarget();
		World->GetTimerManager().SetTimer(BossSearchTimer, this, &UURGPlayerHUD::RefreshBossTarget, 0.25f, true);
	}
}

void UURGPlayerHUD::NativeDestruct()
{
	if (UWorld* World = BossTrackingWorld.Get())
	{
		World->GetTimerManager().ClearTimer(BossSearchTimer);
	}
	BossTrackingWorld.Reset();
	ClearBoss();
	Super::NativeDestruct();
}

void UURGPlayerHUD::RefreshBossTarget()
{
	if (BoundBoss.IsValid() && !BoundBoss->IsActorBeingDestroyed()
		&& BossAttribute.IsValid() && BossAttribute->IsAlive())
	{
		return;
	}
	ClearBoss();
	UWorld* World = GetWorld();
	if (!World) { return; }
	for (TActorIterator<AURGTempleBoss> It(World); It; ++It)
	{
		UURGAttributeComponent* Attribute = It->GetAttributeComponent();
		if (!It->IsActorBeingDestroyed() && Attribute && Attribute->IsAlive())
		{
			BindBoss(*It);
			return;
		}
	}
}

void UURGPlayerHUD::BindBoss(AURGTempleBoss* InBoss)
{
	ClearBoss();
	if (!IsValid(InBoss) || InBoss->IsActorBeingDestroyed()) { return; }
	UURGAttributeComponent* Attribute = InBoss->GetAttributeComponent();
	if (!Attribute || !Attribute->IsAlive()) { return; }
	BoundBoss = InBoss;
	BossAttribute = Attribute;
	BossHealthHandle = Attribute->OnHealthChanged.AddUObject(this, &UURGPlayerHUD::HandleBossHealthChanged);
	BossDeathHandle = Attribute->OnDeath.AddUObject(this, &UURGPlayerHUD::HandleBossDeath);
	HandleBossHealthChanged(Attribute->GetCurrentHealth(), Attribute->GetMaxHealth());
}

void UURGPlayerHUD::ClearBoss()
{
	if (UURGAttributeComponent* Attribute = BossAttribute.Get())
	{
		Attribute->OnHealthChanged.Remove(BossHealthHandle);
		Attribute->OnDeath.Remove(BossDeathHandle);
	}
	BossHealthHandle.Reset();
	BossDeathHandle.Reset();
	BoundBoss.Reset();
	BossAttribute.Reset();
	BossCurrentHealth = BossMaxHealth = 0.0f;
	bShowBossHealth = false;
	InvalidateLayoutAndVolatility();
}

void UURGPlayerHUD::HandleBossHealthChanged(float InCurrent, float InMax)
{
	BossMaxHealth = FMath::Max(0.0f, InMax);
	BossCurrentHealth = FMath::Clamp(InCurrent, 0.0f, BossMaxHealth);
	bShowBossHealth = BossCurrentHealth > 0.0f && BossMaxHealth > 0.0f;
	InvalidateLayoutAndVolatility();
}

void UURGPlayerHUD::HandleBossDeath() { ClearBoss(); }

int32 UURGPlayerHUD::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
	const FSlateRect& Cull, FSlateWindowElementList& Elements, int32 LayerId,
	const FWidgetStyle& Style, bool bParentEnabled) const
{
	const int32 Base = PaintHitFeedback(Geometry, Elements,
		Super::NativePaint(Args, Geometry, Cull, Elements, LayerId, Style, bParentEnabled));
	if (!bShowBossHealth || !BoundBoss.IsValid() || BoundBoss->IsActorBeingDestroyed()) { return Base; }
	const FVector2D Size = Geometry.GetLocalSize();
	const float Width = FMath::Min(720.0f, static_cast<float>(Size.X) * 0.60f);
	if (Width <= 4.0f) { return Base; }
	const float Left = (Size.X - Width) * 0.5f;
	const auto Box = [&](FVector2D Position, FVector2D Extent, FLinearColor Color, int32 Layer)
	{
		FSlateDrawElement::MakeBox(Elements, Base + Layer,
			Geometry.ToPaintGeometry(Extent, FSlateLayoutTransform(Position)),
			FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, Color);
	};
	const auto Text = [&](const FText& Value, FVector2D Position, int32 FontSize)
	{
		FSlateDrawElement::MakeText(Elements, Base + 4,
			Geometry.ToPaintGeometry(FVector2D(1, 1), FSlateLayoutTransform(Position)), Value,
			FCoreStyle::GetDefaultFontStyle("Bold", FontSize), ESlateDrawEffect::None, FLinearColor::White);
	};
	Box(FVector2D(Left - 12, 22), FVector2D(Width + 24, 70), FLinearColor(0.015f, 0.02f, 0.025f, 0.85f), 1);
	Text(BossDisplayName, FVector2D(Left, 27), 20);
	FNumberFormattingOptions Numbers;
	Numbers.SetMaximumFractionalDigits(0);
	Text(FText::Format(NSLOCTEXT("URG.PlayerHUD", "BossHealth", "{0} / {1}"),
		FText::AsNumber(BossCurrentHealth, &Numbers), FText::AsNumber(BossMaxHealth, &Numbers)), FVector2D(Left + Width - 150, 29), 16);
	Box(FVector2D(Left, 58), FVector2D(Width, 22), FLinearColor(0.28f, 0.10f, 0.10f), 2);
	Box(FVector2D(Left + 2, 60), FVector2D(Width - 4, 18), FLinearColor(0.035f, 0.035f, 0.04f), 3);
	const float Ratio = FMath::Clamp(BossCurrentHealth / BossMaxHealth, 0.0f, 1.0f);
	Box(FVector2D(Left + 2, 60), FVector2D((Width - 4) * Ratio, 18), FLinearColor(0.85f, 0.08f, 0.10f), 4);
	return Base + 4;
}

int32 UURGPlayerHUD::PaintHitFeedback(const FGeometry& InGeometry, FSlateWindowElementList& OutElements, const int32 InLayerId) const
{
	const float Now = GetFeedbackTime();
	const int32 NumberLayer = InLayerId + 1;
	const int32 MarkerLayer = InLayerId + 2;

	// 데미지 숫자: 맞은 자리에서 떠오르며 끝부분에 흐려진다
	const APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController && !DamageNumbers.IsEmpty())
	{
		const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

		for (const FURGDamageNumber& Number : DamageNumbers)
		{
			const float Progress = (Now - Number.SpawnTime) / DamageNumberDuration;
			if (Progress < 0.0f || Progress >= 1.0f)
			{
				continue;
			}

			// 카메라 뒤에 있으면 투영이 실패한다
			FVector2D ScreenPosition;
			if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, Number.WorldLocation, ScreenPosition, true))
			{
				continue;
			}

			FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold",
				Number.bCrit ? FMath::RoundToInt(DamageNumberFontSize * CritFontScale) : DamageNumberFontSize);
			Font.OutlineSettings.OutlineSize = 2;

			const float Alpha = Progress < DamageNumberFadeStart ? 1.0f : 1.0f - (Progress - DamageNumberFadeStart) / (1.0f - DamageNumberFadeStart);
			Font.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.8f * Alpha);

			FLinearColor Color = Number.bCrit ? CritDamageNumberColor : DamageNumberColor;
			Color.A *= Alpha;

			// 처음에 빠르게 오르다 느려지도록 ease-out
			const float Rise = DamageNumberRise * (1.0f - FMath::Square(1.0f - Progress));
			const FVector2D TextSize = FontMeasure->Measure(Number.Text, Font);
			const FVector2D Position = ScreenPosition + Number.ScreenJitter - FVector2D(TextSize.X * 0.5f, TextSize.Y + Rise);

			FSlateDrawElement::MakeText(OutElements, NumberLayer,
				InGeometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(Position)), Number.Text,
				Font, ESlateDrawEffect::None, Color);
		}
	}

	// 히트 마커: 처치 마커가 켜져 있으면 일반 적중이 이어져도 처치 마커를 유지한다
	const bool bKillMarker = Now - LastKillTime < KillMarkerDuration;
	const float MarkerDuration = bKillMarker ? KillMarkerDuration : HitMarkerDuration;
	const float MarkerElapsed = Now - (bKillMarker ? LastKillTime : LastHitTime);
	if (MarkerDuration <= 0.0f || MarkerElapsed >= MarkerDuration)
	{
		return NumberLayer;
	}

	const float Scale = bKillMarker ? KillMarkerScale : 1.0f;
	const float Inner = HitMarkerGap * Scale;
	const float Outer = Inner + HitMarkerLength * Scale;

	FLinearColor Color = bKillMarker ? KillMarkerColor : HitMarkerColor;
	Color.A *= 1.0f - MarkerElapsed / MarkerDuration;

	// 화면 중앙(크로스헤어)을 둘러싼 대각선 네 획
	const FVector2D Center = InGeometry.GetLocalSize() * 0.5f;
	const FVector2D Diagonals[] = { FVector2D(1.0f, 1.0f), FVector2D(1.0f, -1.0f), FVector2D(-1.0f, 1.0f), FVector2D(-1.0f, -1.0f) };
	for (const FVector2D& Diagonal : Diagonals)
	{
		const FVector2D Direction = Diagonal.GetSafeNormal();
		const TArray<FVector2D> Points = { Center + Direction * Inner, Center + Direction * Outer };
		FSlateDrawElement::MakeLines(OutElements, MarkerLayer, InGeometry.ToPaintGeometry(), Points,
			ESlateDrawEffect::None, Color, true, 2.5f * Scale);
	}

	return MarkerLayer;
}

float UURGPlayerHUD::GetFeedbackTime() const
{
	const UWorld* World = GetWorld();
	return World ? static_cast<float>(World->GetRealTimeSeconds()) : 0.0f;
}

void UURGPlayerHUD::HandleHitDealt(const FVector& InLocation, const float InDamage, const bool bInCrit, const bool bInKilled)
{
	const float Now = GetFeedbackTime();

	LastHitTime = Now;
	if (bInKilled)
	{
		LastKillTime = Now;
	}

	// 다 떠오른 숫자는 여기서 정리한다 (적중이 없을 땐 페인트에서 건너뛰기만 한다)
	DamageNumbers.RemoveAll([this, Now](const FURGDamageNumber& Number)
	{
		return Now - Number.SpawnTime >= DamageNumberDuration;
	});

	if (DamageNumbers.Num() >= MaxDamageNumbers)
	{
		DamageNumbers.RemoveAt(0);
	}

	FURGDamageNumber& Number = DamageNumbers.AddDefaulted_GetRef();
	Number.WorldLocation = InLocation;
	Number.ScreenJitter = FVector2D(FMath::FRandRange(-16.0f, 16.0f), FMath::FRandRange(-8.0f, 8.0f));
	Number.Text = FText::AsNumber(FMath::Max(1, FMath::RoundToInt(InDamage)));
	Number.SpawnTime = Now;
	Number.bCrit = bInCrit;
}

void UURGPlayerHUD::InitializeForPawn(APawn* InPawn)
{
	if (InPawn == nullptr)
	{
		return;
	}

	// 슬롯 표시가 발동 중인 시너지를 읽으므로 부품 구독보다 먼저 잡아 둔다
	PlayerCharacter = Cast<AURGPlayerCharacter>(InPawn);

	// 각 컴포넌트는 구독 직후 현재 값을 한 번 즉시 반영한다 (첫 변경 전까지 빈 화면이 되지 않도록)
	if (UURGAttributeComponent* Attribute = InPawn->FindComponentByClass<UURGAttributeComponent>())
	{
		Attribute->OnHealthChanged.AddUObject(this, &UURGPlayerHUD::HandleHealthChanged);
		HandleHealthChanged(Attribute->GetCurrentHealth(), Attribute->GetMaxHealth());
	}

	if (UURGResourceComponent* Resource = InPawn->FindComponentByClass<UURGResourceComponent>())
	{
		Resource->OnResourceChanged.AddUObject(this, &UURGPlayerHUD::HandleResourceChanged);
		HandleResourceChanged(Resource->GetResource());
	}

	if (UURGPartComponent* Part = InPawn->FindComponentByClass<UURGPartComponent>())
	{
		Part->OnSlotChanged.AddUObject(this, &UURGPlayerHUD::HandleSlotChanged);

		const EURGPartSlot AllSlots[] = { EURGPartSlot::Arm, EURGPartSlot::Leg, EURGPartSlot::Core, EURGPartSlot::Head };
		for (const EURGPartSlot PartSlot : AllSlots)
		{
			HandleSlotChanged(PartSlot, Part->GetSlotState(PartSlot));
		}
	}

	if (UURGCombatComponent* Combat = InPawn->FindComponentByClass<UURGCombatComponent>())
	{
		CombatComponent = Combat;
		Combat->OnWeaponChanged.AddUObject(this, &UURGPlayerHUD::HandleWeaponChanged);
		Combat->OnHitDealt.AddUObject(this, &UURGPlayerHUD::HandleHitDealt);
		HandleWeaponChanged();
	}

	// 코어 능력 발동 문구와 시너지 발동은 컴포넌트가 아니라 캐릭터가 알린다
	if (AURGPlayerCharacter* Player = PlayerCharacter.Get())
	{
		Player->OnCoreAbilityActivated.AddUObject(this, &UURGPlayerHUD::HandleCoreAbilityActivated);
		Player->OnCoreAbilityStateChanged.AddUObject(this, &UURGPlayerHUD::HandleCoreAbilityStateChanged);
		Player->OnDashCooldownChanged.AddUObject(this, &UURGPlayerHUD::HandleDashCooldownChanged);
		Player->OnAttackCooldownChanged.AddUObject(this, &UURGPlayerHUD::HandleAttackCooldownChanged);
		Player->OnSynergyChanged.AddUObject(this, &UURGPlayerHUD::HandleSynergyChanged);
		Player->OnInteractFocusChanged.AddUObject(this, &UURGPlayerHUD::HandleInteractFocusChanged);
	}

	if (UURGRadarComponent* Radar = InPawn->FindComponentByClass<UURGRadarComponent>())
	{
		Radar->OnRadarUpdated.AddUObject(this, &UURGPlayerHUD::HandleRadarUpdated);
	}

	// 시작 시 이미 발동 중인 시너지(스테이지 이동 복원 등)는 알림 없이 패널에만 반영한다
	RefreshSynergyPanel();
	RefreshPartCompare();
}

void UURGPlayerHUD::HandleRadarUpdated(const TArray<FVector>& InEnemyLocations)
{
	if (EnemyRadarUI)
	{
		EnemyRadarUI->SetTargets(InEnemyLocations);
	}
}

void UURGPlayerHUD::HandleInteractFocusChanged(AActor* InActor)
{
	// 수리 장치 등 다른 상호작용 대상이면 비교 UI는 숨긴다
	FocusedPickup = Cast<AURGPartPickup>(InActor);
	RefreshPartCompare();
}

void UURGPlayerHUD::RefreshPartCompare() const
{
	if (PartCompareUI == nullptr)
	{
		return;
	}

	const AURGPartPickup* Pickup = FocusedPickup.Get();
	const UURGPartDataAsset* NewPart = Pickup ? Pickup->GetPart() : nullptr;
	const AURGPlayerCharacter* Player = PlayerCharacter.Get();
	const UURGPartComponent* Part = Player ? Player->GetPartComponent() : nullptr;
	if (NewPart == nullptr || Part == nullptr)
	{
		PartCompareUI->HideCompare();
		return;
	}

	const FURGPartSlotState& Current = Part->GetSlotState(NewPart->GetSlot());

	// 장착하면 켜지거나 꺼지는 시너지만 보여 준다 (그대로인 건 판단에 필요 없다)
	const FGameplayTag DisplayOrder[] = { URGGameplayTags::Part_Tag_Reinforce, URGGameplayTags::Part_Tag_Thermal, URGGameplayTags::Part_Tag_Electric };

	TArray<FText> PreviewLines;
	for (const FGameplayTag& Tag : DisplayOrder)
	{
		const bool bActiveNow = Player->GetActiveSynergies().HasTagExact(Tag);
		const bool bActiveAfter = Part->IsSynergyActiveWithPart(Tag, NewPart);

		if (!bActiveNow && bActiveAfter)
		{
			PreviewLines.Add(FText::Format(PreviewOnFormat, GetSynergyTagName(Tag), BuildSynergyDescription(Tag)));
		}
		else if (bActiveNow && !bActiveAfter)
		{
			PreviewLines.Add(FText::Format(PreviewOffFormat, GetSynergyTagName(Tag)));
		}
	}

	PartCompareUI->ShowCompare(
		NewPart,
		Current,
		BuildTagNamesText(NewPart->GetSynergyTags()),
		FText::Join(FText::FromString(TEXT("\n")), PreviewLines));
}

FText UURGPlayerHUD::BuildTagNamesText(const FGameplayTagContainer& InTags) const
{
	TArray<FText> TagNames;
	for (const FGameplayTag& Tag : InTags)
	{
		const FText& TagName = GetSynergyTagName(Tag);
		if (!TagName.IsEmpty())
		{
			TagNames.Add(TagName);
		}
	}

	return FText::Join(SynergyTagSeparator, TagNames);
}

void UURGPlayerHUD::HandleSynergyChanged(const FGameplayTag& InTag, const bool bInActive)
{
	const FText& TagName = GetSynergyTagName(InTag);
	if (!TagName.IsEmpty())
	{
		// 켜질 때는 무엇이 좋아지는지까지 알려 준다
		const FText Message = bInActive
			? FText::Format(SynergyOnFormat, TagName, BuildSynergyDescription(InTag))
			: FText::Format(SynergyOffFormat, TagName);
		ShowCommMessage(Message);
	}

	RefreshSynergyPanel();

	// 한 슬롯이 바뀌어도 같은 태그를 가진 다른 슬롯의 강조가 함께 바뀐다
	const AURGPlayerCharacter* Player = PlayerCharacter.Get();
	const UURGPartComponent* Part = Player ? Player->GetPartComponent() : nullptr;
	if (Part == nullptr)
	{
		return;
	}

	for (const TPair<EURGPartSlot, FURGPartSlotState>& Pair : Part->GetSlots())
	{
		RefreshSlotSynergyTags(Pair.Key, Pair.Value);
	}
}

void UURGPlayerHUD::RefreshSlotSynergyTags(const EURGPartSlot InSlot, const FURGPartSlotState& InState) const
{
	if (PartSlotsUI == nullptr)
	{
		return;
	}

	if (!InState.IsEquipped())
	{
		PartSlotsUI->SetSlotSynergyTags(InSlot, FText::GetEmpty(), SynergyInactiveColor);
		return;
	}

	const FText TagNames = BuildTagNamesText(InState.Part->GetSynergyTags());
	if (TagNames.IsEmpty())
	{
		PartSlotsUI->SetSlotSynergyTags(InSlot, FText::GetEmpty(), SynergyInactiveColor);
		return;
	}

	// 파손 부품은 시너지 개수에 들어가지 않으므로 태그가 발동 중이어도 강조하지 않는다
	const AURGPlayerCharacter* Player = PlayerCharacter.Get();
	const bool bUsable = InState.Condition != EURGPartCondition::Broken;

	// 발동 중인 태그가 있으면 그 태그 색, 없으면 회색. 패널의 같은 태그 줄과 색이 맞아 어느 부품이 시너지를 이루는지 보인다
	const FLinearColor* TagColor = &SynergyInactiveColor;
	for (const FGameplayTag& Tag : InState.Part->GetSynergyTags())
	{
		if (bUsable && Player && Player->GetActiveSynergies().HasTagExact(Tag))
		{
			TagColor = &GetSynergyTagColor(Tag);
			break;
		}
	}

	PartSlotsUI->SetSlotSynergyTags(InSlot, FText::Format(SlotTagFormat, TagNames), *TagColor);
}

const FText& UURGPlayerHUD::GetSynergyTagName(const FGameplayTag& InTag) const
{
	if (InTag.MatchesTagExact(URGGameplayTags::Part_Tag_Thermal))
	{
		return ThermalTagName;
	}

	if (InTag.MatchesTagExact(URGGameplayTags::Part_Tag_Electric))
	{
		return ElectricTagName;
	}

	if (InTag.MatchesTagExact(URGGameplayTags::Part_Tag_Reinforce))
	{
		return ReinforceTagName;
	}

	return FText::GetEmpty();
}

const FLinearColor& UURGPlayerHUD::GetSynergyTagColor(const FGameplayTag& InTag) const
{
	if (InTag.MatchesTagExact(URGGameplayTags::Part_Tag_Thermal))
	{
		return ThermalTagColor;
	}

	if (InTag.MatchesTagExact(URGGameplayTags::Part_Tag_Electric))
	{
		return ElectricTagColor;
	}

	if (InTag.MatchesTagExact(URGGameplayTags::Part_Tag_Reinforce))
	{
		return ReinforceTagColor;
	}

	return SynergyInactiveColor;
}

FText UURGPlayerHUD::BuildSynergyPips(const int32 InCount, const int32 InThreshold) const
{
	// 필요 개수를 넘게 모아도 점은 필요 개수만큼만 그린다
	const int32 Filled = FMath::Clamp(InCount, 0, InThreshold);
	const FString FilledPip = SynergyFilledPip.ToString();
	const FString EmptyPip = SynergyEmptyPip.ToString();

	FString Pips;
	for (int32 Index = 0; Index < InThreshold; ++Index)
	{
		Pips += Index < Filled ? FilledPip : EmptyPip;
	}

	return FText::FromString(Pips);
}

FText UURGPlayerHUD::BuildSynergyDescription(const FGameplayTag& InTag) const
{
	// 0.5초 같은 값이 0.500으로 나오지 않도록 소수 한 자리까지만
	FNumberFormattingOptions NumberOptions;
	NumberOptions.MinimumFractionalDigits = 0;
	NumberOptions.MaximumFractionalDigits = 1;

	if (InTag.MatchesTagExact(URGGameplayTags::Part_Tag_Thermal))
	{
		const UURGCombatComponent* Combat = CombatComponent.Get();
		if (Combat == nullptr)
		{
			return FText::GetEmpty();
		}

		return FText::Format(ThermalDescFormat,
			FText::AsNumber(Combat->GetBurnDuration(), &NumberOptions),
			FText::AsNumber(Combat->GetBurnTickInterval(), &NumberOptions),
			FText::AsNumber(Combat->GetBurnDamagePerTick(), &NumberOptions));
	}

	if (InTag.MatchesTagExact(URGGameplayTags::Part_Tag_Electric))
	{
		const UURGCombatComponent* Combat = CombatComponent.Get();
		if (Combat == nullptr)
		{
			return FText::GetEmpty();
		}

		// 반경은 cm → m, 비율은 0~1 → %
		return FText::Format(ElectricDescFormat,
			FText::AsNumber(Combat->GetChainRadius() / 100.0f, &NumberOptions),
			FText::AsNumber(Combat->GetChainMaxTargets()),
			FText::AsNumber(FMath::RoundToInt(Combat->GetChainDamageRatio() * 100.0f)));
	}

	if (InTag.MatchesTagExact(URGGameplayTags::Part_Tag_Reinforce))
	{
		const AURGPlayerCharacter* Player = PlayerCharacter.Get();
		if (Player == nullptr)
		{
			return FText::GetEmpty();
		}

		// 손상 배율 0.7 = 30% 덜 손상
		return FText::Format(ReinforceDescFormat,
			FText::AsNumber(Player->GetReinforceMaxHealthBonus(), &NumberOptions),
			FText::AsNumber(FMath::RoundToInt((1.0f - Player->GetReinforcePartDamageScale()) * 100.0f)));
	}

	return FText::GetEmpty();
}

void UURGPlayerHUD::RefreshSynergyPanel() const
{
	const AURGPlayerCharacter* Player = PlayerCharacter.Get();
	const UURGPartComponent* Part = Player ? Player->GetPartComponent() : nullptr;
	if (SynergyPanelUI == nullptr || Part == nullptr)
	{
		return;
	}

	// 태그 컨테이너 순서는 추가 순서라 흔들리므로 표시 순서를 고정한다
	const FGameplayTag DisplayOrder[] = { URGGameplayTags::Part_Tag_Reinforce, URGGameplayTags::Part_Tag_Thermal, URGGameplayTags::Part_Tag_Electric };

	// 줄 위치가 태그마다 고정이라 발동 여부가 바뀌어도 줄이 뛰지 않는다. 하나라도 모은 태그는 진행 점으로 무엇을 더 모으면 되는지 보여 준다
	const int32 Threshold = Part->GetSynergyThreshold();
	for (const FGameplayTag& Tag : DisplayOrder)
	{
		if (Player->GetActiveSynergies().HasTagExact(Tag))
		{
			const FText Row = FText::Format(SynergyActiveRowFormat, GetSynergyTagName(Tag), BuildSynergyPips(Threshold, Threshold), BuildSynergyDescription(Tag));
			SynergyPanelUI->SetSynergyRow(Tag, Row, GetSynergyTagColor(Tag));
			continue;
		}

		const int32 Count = Part->GetSynergyTagCount(Tag);
		const FText Row = Count > 0
			? FText::Format(SynergyProgressRowFormat, GetSynergyTagName(Tag), BuildSynergyPips(Count, Threshold))
			: FText::GetEmpty();
		SynergyPanelUI->SetSynergyRow(Tag, Row, SynergyInactiveColor);
	}
}

void UURGPlayerHUD::HandleCoreAbilityActivated(const FText& InMessage)
{
	if (!InMessage.IsEmpty())
	{
		ShowCommMessage(InMessage);
	}
}

void UURGPlayerHUD::HandleCoreAbilityStateChanged(const bool bInActive, const float InCooldownEndTime, const float InCooldownDuration)
{
	if (PartSlotsUI)
	{
		PartSlotsUI->SetCoreAbilityState(bInActive, InCooldownEndTime, InCooldownDuration);
	}
}

void UURGPlayerHUD::HandleDashCooldownChanged(const float InCooldownEndTime, const float InCooldownDuration)
{
	if (PartSlotsUI)
	{
		PartSlotsUI->SetSlotCooldownState(EURGPartSlot::Leg, InCooldownEndTime, InCooldownDuration);
	}
}

void UURGPlayerHUD::HandleAttackCooldownChanged(const float InCooldownEndTime, const float InCooldownDuration)
{
	if (PartSlotsUI)
	{
		PartSlotsUI->SetSlotCooldownState(EURGPartSlot::Arm, InCooldownEndTime, InCooldownDuration);
	}
}

void UURGPlayerHUD::ShowCommMessage(const FText& InMessage, const float InDuration)
{
	if (CommLogUI)
	{
		CommLogUI->ShowMessage(InMessage, InDuration);
	}
}

void UURGPlayerHUD::SetStageInfo(const int32 InStage, const int32 InRoom, const int32 InTotalRooms)
{
	if (StageInfoUI)
	{
		StageInfoUI->SetStageInfo(InStage, InRoom, InTotalRooms);
	}
}

void UURGPlayerHUD::HandleHealthChanged(const float InCurrent, const float InMax) const
{
	if (HealthBarUI)
	{
		HealthBarUI->SetRatio(GetSafeRatio(InCurrent, InMax));
		HealthBarUI->SetText(InCurrent, InMax);
	}
}

void UURGPlayerHUD::HandleResourceChanged(const int32 InResource) const
{
	if (ResourceUI)
	{
		ResourceUI->SetResource(InResource);
	}
}

void UURGPlayerHUD::HandleSlotChanged(const EURGPartSlot InSlot, const FURGPartSlotState& InState)
{
	NotifyPartConditionChange(InSlot, InState);

	if (PartSlotsUI)
	{
		PartSlotsUI->SetSlotState(InSlot, InState);
	}

	// 캐릭터가 먼저 구독해 시너지를 다시 계산했으므로 여기서 읽는 발동 상태는 이미 최신이다
	RefreshSlotSynergyTags(InSlot, InState);

	// 발동이 바뀌지 않아도 모은 개수는 바뀔 수 있다 (1개 장착·파손)
	RefreshSynergyPanel();

	// 픽업을 보고 있는 중에 맞아서 현재 부품 상태가 바뀌면 비교 내용도 따라 바뀐다
	if (FocusedPickup.IsValid())
	{
		RefreshPartCompare();
	}
}

void UURGPlayerHUD::NotifyPartConditionChange(const EURGPartSlot InSlot, const FURGPartSlotState& InState)
{
	// 같은 부품이 더 나쁜 단계로 갔을 때만 알린다. 첫 반영(HUD 생성·스테이지 이동 복원), 부품 교체, 수리는 기록만 갱신한다
	const FURGSlotConditionRecord* Last = LastSlotConditions.Find(InSlot);
	const bool bWorsened = Last && InState.IsEquipped()
		&& Last->Part.Get() == InState.Part
		&& InState.Condition > Last->Condition;

	FURGSlotConditionRecord& Record = LastSlotConditions.FindOrAdd(InSlot);
	Record.Part = InState.Part;
	Record.Condition = InState.Condition;

	if (!bWorsened)
	{
		return;
	}

	// 큰 피해로 정상에서 바로 파손될 수도 있으므로 지금 단계로 문구를 고른다
	if (InState.Condition == EURGPartCondition::Broken)
	{
		const FText* Effect = BrokenEffectTexts.Find(InSlot);
		ShowCommMessage(FText::Format(PartBrokenFormat, InState.GetDisplayName(), Effect ? *Effect : FText::GetEmpty()), PartConditionMessageDuration);
		return;
	}

	ShowCommMessage(FText::Format(PartDamagedFormat, InState.GetDisplayName()), PartConditionMessageDuration);
}

void UURGPlayerHUD::HandleAmmoChanged(const int32 InCurrent, const int32 InMagazineSize) const
{
	if (AmmoUI)
	{
		AmmoUI->SetAmmo(InCurrent, InMagazineSize);
	}
}

void UURGPlayerHUD::HandleWeaponChanged()
{
	// 이전 무기에 걸어둔 구독을 해제한다
	if (AURGRangedWeapon* Previous = BoundWeapon.Get())
	{
		Previous->OnAmmoChanged.RemoveAll(this);
	}
	BoundWeapon = nullptr;

	const UURGCombatComponent* Combat = CombatComponent.Get();
	AURGRangedWeapon* Weapon = Combat ? Cast<AURGRangedWeapon>(Combat->GetMainWeapon()) : nullptr;

	if (Weapon == nullptr)
	{
		HandleAmmoChanged(0, 0); // 무기가 없거나 탄약을 쓰지 않는 무기(근접)면 0 / 0 → 탄약 UI 숨김
		return;
	}

	Weapon->OnAmmoChanged.AddUObject(this, &UURGPlayerHUD::HandleAmmoChanged);
	BoundWeapon = Weapon;

	// 현재 탄약을 즉시 한 번 반영
	HandleAmmoChanged(Weapon->GetCurrentAmmo(), Weapon->GetMagazineSize());
}

float UURGPlayerHUD::GetSafeRatio(const float InCurrent, const float InMax) const
{
	return InMax > 0.0f ? InCurrent / InMax : 0.0f;
}


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
#include "Component/URGCoreAbilityComponent.h"
#include "Component/URGDashComponent.h"
#include "Component/URGInteractComponent.h"
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
#include "UI/URGHitFeedbackUI.h"
#include "UI/URGPartCompareUI.h"
#include "UI/URGPartSlotsUI.h"
#include "UI/URGResourceUI.h"
#include "UI/URGStageInfoUI.h"
#include "UI/URGSynergyPanelUI.h"
#include "URGGameplayTags.h"

UURGPlayerHUD::UURGPlayerHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 네이티브 태그는 cpp에서만 보이므로 표시 정보 기본값은 헤더 대신 여기서 채운다
	SynergyDisplays = {
		{ URGGameplayTags::Part_Tag_Reinforce, NSLOCTEXT("URG.PlayerHUD", "ReinforceTag", "강화"), FLinearColor(0.7f, 0.55f, 1.0f, 1.0f) },
		{ URGGameplayTags::Part_Tag_Thermal, NSLOCTEXT("URG.PlayerHUD", "ThermalTag", "열"), FLinearColor(1.0f, 0.4f, 0.1f, 1.0f) },
		{ URGGameplayTags::Part_Tag_Electric, NSLOCTEXT("URG.PlayerHUD", "ElectricTag", "전기"), FLinearColor(0.3f, 0.85f, 1.0f, 1.0f) },
	};
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
	UnbindFromPawn();
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
	const int32 Base = Super::NativePaint(Args, Geometry, Cull, Elements, LayerId, Style, bParentEnabled);
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

void UURGPlayerHUD::HandleHitDealt(const FVector& InLocation, float InDamage, bool bInCrit, bool bInKilled)
{
	if (HitFeedbackUI != nullptr)
	{
		HitFeedbackUI->AddHit(InLocation, InDamage, bInCrit, bInKilled);
	}
}

void UURGPlayerHUD::InitializeForPawn(APawn* InPawn)
{
	if (InPawn == nullptr)
	{
		return;
	}

	// 다른 폰에 다시 붙는 경우 옛 폰 구독이 남지 않도록 먼저 푼다
	UnbindFromPawn();
	BoundPawn = InPawn;

	// 강화 시너지 설명 수치는 캐릭터가 들고 있다
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
		// 슬롯 표시가 발동 중인 시너지를 읽으므로 아래 첫 반영보다 먼저 잡아 둔다
		PartComponent = Part;
		Part->OnSlotChanged.AddUObject(this, &UURGPlayerHUD::HandleSlotChanged);
		Part->OnSynergyChanged.AddUObject(this, &UURGPlayerHUD::HandleSynergyChanged);

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
		Combat->OnAttackCooldownChanged.AddUObject(this, &UURGPlayerHUD::HandleAttackCooldownChanged);
		HandleWeaponChanged();
	}

	// 대시 쿨다운은 대시를 쓸 때·다리를 바꿀 때만 오므로 시작 시 반영할 값이 없다
	if (UURGDashComponent* Dash = InPawn->FindComponentByClass<UURGDashComponent>())
	{
		Dash->OnDashCooldownChanged.AddUObject(this, &UURGPlayerHUD::HandleDashCooldownChanged);
	}

	if (UURGInteractComponent* Interact = InPawn->FindComponentByClass<UURGInteractComponent>())
	{
		Interact->OnInteractFocusChanged.AddUObject(this, &UURGPlayerHUD::HandleInteractFocusChanged);
	}

	if (UURGCoreAbilityComponent* CoreAbility = InPawn->FindComponentByClass<UURGCoreAbilityComponent>())
	{
		CoreAbility->OnCoreAbilityActivated.AddUObject(this, &UURGPlayerHUD::HandleCoreAbilityActivated);
		CoreAbility->OnCoreAbilityStateChanged.AddUObject(this, &UURGPlayerHUD::HandleCoreAbilityStateChanged);
	}

	if (UURGRadarComponent* Radar = InPawn->FindComponentByClass<UURGRadarComponent>())
	{
		Radar->OnRadarUpdated.AddUObject(this, &UURGPlayerHUD::HandleRadarUpdated);
	}

	// 시작 시 이미 발동 중인 시너지(스테이지 이동 복원 등)는 알림 없이 패널에만 반영한다
	RefreshSynergyPanel();
	RefreshPartCompare();
}

void UURGPlayerHUD::UnbindFromPawn()
{
	// 탄약은 폰이 아니라 무기 액터가 쏘므로 따로 푼다
	if (AURGRangedWeapon* Weapon = BoundWeapon.Get())
	{
		Weapon->OnAmmoChanged.RemoveAll(this);
	}
	BoundWeapon.Reset();

	// 컴포넌트를 멤버로 다 들고 있지 않고, 묶을 때 쓴 폰에서 다시 찾는다 (한 번뿐인 정리라 비용은 문제 되지 않는다)
	if (APawn* Pawn = BoundPawn.Get())
	{
		if (UURGAttributeComponent* Attribute = Pawn->FindComponentByClass<UURGAttributeComponent>())
		{
			Attribute->OnHealthChanged.RemoveAll(this);
		}

		if (UURGResourceComponent* Resource = Pawn->FindComponentByClass<UURGResourceComponent>())
		{
			Resource->OnResourceChanged.RemoveAll(this);
		}

		if (UURGPartComponent* Part = Pawn->FindComponentByClass<UURGPartComponent>())
		{
			Part->OnSlotChanged.RemoveAll(this);
			Part->OnSynergyChanged.RemoveAll(this);
		}

		if (UURGCombatComponent* Combat = Pawn->FindComponentByClass<UURGCombatComponent>())
		{
			Combat->OnWeaponChanged.RemoveAll(this);
			Combat->OnHitDealt.RemoveAll(this);
			Combat->OnAttackCooldownChanged.RemoveAll(this);
		}

		if (UURGDashComponent* Dash = Pawn->FindComponentByClass<UURGDashComponent>())
		{
			Dash->OnDashCooldownChanged.RemoveAll(this);
		}

		if (UURGInteractComponent* Interact = Pawn->FindComponentByClass<UURGInteractComponent>())
		{
			Interact->OnInteractFocusChanged.RemoveAll(this);
		}

		if (UURGCoreAbilityComponent* CoreAbility = Pawn->FindComponentByClass<UURGCoreAbilityComponent>())
		{
			CoreAbility->OnCoreAbilityActivated.RemoveAll(this);
			CoreAbility->OnCoreAbilityStateChanged.RemoveAll(this);
		}

		if (UURGRadarComponent* Radar = Pawn->FindComponentByClass<UURGRadarComponent>())
		{
			Radar->OnRadarUpdated.RemoveAll(this);
		}
	}

	BoundPawn.Reset();
	PlayerCharacter.Reset();
	PartComponent.Reset();
	CombatComponent.Reset();
	FocusedPickup.Reset();
	LastSlotConditions.Reset();
}

void UURGPlayerHUD::HandleRadarUpdated(const TArray<FVector>& InEnemyLocations)
{
	if (EnemyRadarUI != nullptr)
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
	const UURGPartDataAsset* NewPart = Pickup != nullptr ? Pickup->GetPart() : nullptr;
	const UURGPartComponent* Part = PartComponent.Get();
	if (NewPart == nullptr || Part == nullptr)
	{
		PartCompareUI->HideCompare();
		return;
	}

	const FURGPartSlotState& Current = Part->GetSlotState(NewPart->GetSlot());

	// 장착하면 켜지거나 꺼지는 시너지만 보여 준다 (그대로인 건 판단에 필요 없다)
	TArray<FText> PreviewLines;
	for (const FURGSynergyDisplay& Display : SynergyDisplays)
	{
		const bool bActiveNow = Part->IsSynergyActive(Display.Tag);
		const bool bActiveAfter = Part->IsSynergyActiveWithPart(Display.Tag, NewPart);

		if (!bActiveNow && bActiveAfter)
		{
			PreviewLines.Add(FText::Format(PreviewOnFormat, Display.Name, BuildSynergyDescription(Display.Tag)));
		}
		else if (bActiveNow && !bActiveAfter)
		{
			PreviewLines.Add(FText::Format(PreviewOffFormat, Display.Name));
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

void UURGPlayerHUD::HandleSynergyChanged(const FGameplayTag& InTag, bool bInActive)
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
	const UURGPartComponent* Part = PartComponent.Get();
	if (Part == nullptr)
	{
		return;
	}

	for (const TPair<EURGPartSlot, FURGPartSlotState>& Pair : Part->GetSlots())
	{
		RefreshSlotSynergyTags(Pair.Key, Pair.Value);
	}
}

void UURGPlayerHUD::RefreshSlotSynergyTags(EURGPartSlot InSlot, const FURGPartSlotState& InState) const
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
	const UURGPartComponent* Part = PartComponent.Get();
	const bool bUsable = InState.Condition != EURGPartCondition::Broken;

	// 발동 중인 태그가 있으면 그 태그 색, 없으면 회색. 패널의 같은 태그 줄과 색이 맞아 어느 부품이 시너지를 이루는지 보인다
	const FLinearColor* TagColor = &SynergyInactiveColor;
	for (const FGameplayTag& Tag : InState.Part->GetSynergyTags())
	{
		if (bUsable && Part != nullptr && Part->IsSynergyActive(Tag))
		{
			TagColor = &GetSynergyTagColor(Tag);
			break;
		}
	}

	PartSlotsUI->SetSlotSynergyTags(InSlot, FText::Format(SlotTagFormat, TagNames), *TagColor);
}

const FURGSynergyDisplay* UURGPlayerHUD::FindSynergyDisplay(const FGameplayTag& InTag) const
{
	return SynergyDisplays.FindByPredicate([&InTag](const FURGSynergyDisplay& Display)
	{
		return Display.Tag.MatchesTagExact(InTag);
	});
}

const FText& UURGPlayerHUD::GetSynergyTagName(const FGameplayTag& InTag) const
{
	const FURGSynergyDisplay* Display = FindSynergyDisplay(InTag);
	return Display != nullptr ? Display->Name : FText::GetEmpty();
}

const FLinearColor& UURGPlayerHUD::GetSynergyTagColor(const FGameplayTag& InTag) const
{
	const FURGSynergyDisplay* Display = FindSynergyDisplay(InTag);
	return Display != nullptr ? Display->Color : SynergyInactiveColor;
}

FText UURGPlayerHUD::BuildSynergyPips(int32 InCount, int32 InThreshold) const
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
	const UURGPartComponent* Part = PartComponent.Get();
	if (SynergyPanelUI == nullptr || Part == nullptr)
	{
		return;
	}

	// 태그 컨테이너 순서는 추가 순서라 흔들리므로 SynergyDisplays 순서로 고정한다.
	// 줄 위치가 태그마다 고정이라 발동 여부가 바뀌어도 줄이 뛰지 않는다. 하나라도 모은 태그는 진행 점으로 무엇을 더 모으면 되는지 보여 준다
	const int32 Threshold = Part->GetSynergyThreshold();
	for (const FURGSynergyDisplay& Display : SynergyDisplays)
	{
		if (Part->IsSynergyActive(Display.Tag))
		{
			const FText Row = FText::Format(SynergyActiveRowFormat, Display.Name, BuildSynergyPips(Threshold, Threshold), BuildSynergyDescription(Display.Tag));
			SynergyPanelUI->SetSynergyRow(Display.Tag, Row, Display.Color);
			continue;
		}

		const int32 Count = Part->GetSynergyTagCount(Display.Tag);
		const FText Row = Count > 0
			? FText::Format(SynergyProgressRowFormat, Display.Name, BuildSynergyPips(Count, Threshold))
			: FText::GetEmpty();
		SynergyPanelUI->SetSynergyRow(Display.Tag, Row, SynergyInactiveColor);
	}
}

void UURGPlayerHUD::HandleCoreAbilityActivated(const FText& InMessage)
{
	if (!InMessage.IsEmpty())
	{
		ShowCommMessage(InMessage);
	}
}

void UURGPlayerHUD::HandleCoreAbilityStateChanged(bool bInActive, float InCooldownEndTime, float InCooldownDuration)
{
	if (PartSlotsUI != nullptr)
	{
		PartSlotsUI->SetCoreAbilityState(bInActive, InCooldownEndTime, InCooldownDuration);
	}
}

void UURGPlayerHUD::HandleDashCooldownChanged(float InCooldownEndTime, float InCooldownDuration)
{
	if (PartSlotsUI != nullptr)
	{
		PartSlotsUI->SetSlotCooldownState(EURGPartSlot::Leg, InCooldownEndTime, InCooldownDuration);
	}
}

void UURGPlayerHUD::HandleAttackCooldownChanged(float InCooldownEndTime, float InCooldownDuration)
{
	if (PartSlotsUI != nullptr)
	{
		PartSlotsUI->SetSlotCooldownState(EURGPartSlot::Arm, InCooldownEndTime, InCooldownDuration);
	}
}

void UURGPlayerHUD::ShowCommMessage(const FText& InMessage, float InDuration)
{
	if (CommLogUI != nullptr)
	{
		CommLogUI->ShowMessage(InMessage, InDuration);
	}
}

void UURGPlayerHUD::SetStageInfo(int32 InStage, int32 InRoom, int32 InTotalRooms)
{
	if (StageInfoUI != nullptr)
	{
		StageInfoUI->SetStageInfo(InStage, InRoom, InTotalRooms);
	}
}

void UURGPlayerHUD::HandleHealthChanged(float InCurrent, float InMax) const
{
	if (HealthBarUI != nullptr)
	{
		HealthBarUI->SetHealth(InCurrent, InMax);
	}
}

void UURGPlayerHUD::HandleResourceChanged(int32 InResource) const
{
	if (ResourceUI != nullptr)
	{
		ResourceUI->SetResource(InResource);
	}
}

void UURGPlayerHUD::HandleSlotChanged(EURGPartSlot InSlot, const FURGPartSlotState& InState)
{
	NotifyPartConditionChange(InSlot, InState);

	if (PartSlotsUI != nullptr)
	{
		PartSlotsUI->SetSlotState(InSlot, InState);
	}

	// 부품 컴포넌트가 방송 전에 시너지 기록을 고치므로 여기서 읽는 발동 상태는 이미 최신이다 (구독 순서와 무관)
	RefreshSlotSynergyTags(InSlot, InState);

	// 발동이 바뀌지 않아도 모은 개수는 바뀔 수 있다 (1개 장착·파손)
	RefreshSynergyPanel();

	// 픽업을 보고 있는 중에 맞아서 현재 부품 상태가 바뀌면 비교 내용도 따라 바뀐다
	if (FocusedPickup.IsValid())
	{
		RefreshPartCompare();
	}
}

void UURGPlayerHUD::NotifyPartConditionChange(EURGPartSlot InSlot, const FURGPartSlotState& InState)
{
	// 같은 부품이 더 나쁜 단계로 갔을 때만 알린다. 첫 반영(HUD 생성·스테이지 이동 복원), 부품 교체, 수리는 기록만 갱신한다
	const FURGSlotConditionRecord* Last = LastSlotConditions.Find(InSlot);
	const bool bWorsened = Last != nullptr && InState.IsEquipped()
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
		ShowCommMessage(FText::Format(PartBrokenFormat, InState.GetDisplayName(), Effect != nullptr ? *Effect : FText::GetEmpty()), PartConditionMessageDuration);
		return;
	}

	ShowCommMessage(FText::Format(PartDamagedFormat, InState.GetDisplayName()), PartConditionMessageDuration);
}

void UURGPlayerHUD::HandleAmmoChanged(int32 InCurrent, int32 InMagazineSize) const
{
	if (AmmoUI != nullptr)
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
	AURGRangedWeapon* Weapon = Combat != nullptr ? Cast<AURGRangedWeapon>(Combat->GetMainWeapon()) : nullptr;

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


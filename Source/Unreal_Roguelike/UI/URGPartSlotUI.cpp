#include "UI/URGPartSlotUI.h"
#include "Component/URGPartComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Equipment/URGPartDataAsset.h"
#include "Materials/MaterialInstanceDynamic.h"

void UURGPartSlotUI::NativeConstruct()
{
	Super::NativeConstruct();

	// 능력이 없는 슬롯과 아직 능력을 쓰지 않은 슬롯은 쿨다운 표시가 없다
	UpdateAbilityVisuals(0.0f);
}

void UURGPartSlotUI::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (CooldownEndTime <= 0.0f)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const float Remaining = CooldownEndTime - World->GetTimeSeconds();
	UpdateAbilityVisuals(Remaining);

	// 끝났으면 더 갱신하지 않도록 쿨다운을 비운다
	if (Remaining <= 0.0f)
	{
		CooldownEndTime = 0.0f;
	}
}

void UURGPartSlotUI::SetAbilityState(const bool bInActive, const float InCooldownEndTime, const float InCooldownDuration)
{
	bAbilityActive = bInActive;
	CooldownEndTime = InCooldownDuration > 0.0f ? InCooldownEndTime : 0.0f;
	CooldownDuration = InCooldownDuration;

	const UWorld* World = GetWorld();
	const float Remaining = (CooldownEndTime > 0.0f && World) ? CooldownEndTime - World->GetTimeSeconds() : 0.0f;
	UpdateAbilityVisuals(Remaining);
}

void UURGPartSlotUI::UpdateAbilityVisuals(const float InCooldownRemaining) const
{
	const bool bCoolingDown = InCooldownRemaining > 0.0f && CooldownDuration > 0.0f;

	if (CooldownArc)
	{
		if (bCoolingDown)
		{
			if (UMaterialInstanceDynamic* ArcMaterial = CooldownArc->GetDynamicMaterial())
			{
				static const FName PercentParameterName(TEXT("Percent"));
				static const FName ColorParameterName(TEXT("Color"));

				// 남은 비율만큼 호가 남아 있다가 0이 되면 준비 완료
				ArcMaterial->SetScalarParameterValue(PercentParameterName, InCooldownRemaining / CooldownDuration);
				ArcMaterial->SetVectorParameterValue(ColorParameterName, CooldownArcColor);
			}
		}
		CooldownArc->SetVisibility(bCoolingDown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (CooldownText)
	{
		if (bCoolingDown)
		{
			// 소수점은 매 프레임 바뀌어 읽기 어려우므로 올림한 정수 초로 보여 준다
			CooldownText->SetText(FText::AsNumber(FMath::CeilToInt(InCooldownRemaining)));
		}
		CooldownText->SetVisibility(bCoolingDown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (PartIcon)
	{
		if (bBroken)
		{
			PartIcon->SetColorAndOpacity(BrokenIconTint);
		}
		else
		{
			PartIcon->SetColorAndOpacity(bCoolingDown ? CooldownIconTint : FLinearColor::White);
		}
	}

	if (ActiveGlow)
	{
		ActiveGlow->SetVisibility(bAbilityActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UURGPartSlotUI::SetSlotState(const FURGPartSlotState& InState, const bool bInArmSlot)
{
	const bool bIsEquipped = InState.IsEquipped();
	bBroken = bIsEquipped && InState.Condition == EURGPartCondition::Broken;

	if (DurabilityArc)
	{
		if (UMaterialInstanceDynamic* ArcMaterial = DurabilityArc->GetDynamicMaterial())
		{
			static const FName PercentParameterName(TEXT("Percent"));
			static const FName ColorParameterName(TEXT("Color"));

			// 손상 단계를 색으로 바꾸면 슬롯 구분과 섞이므로, 색은 슬롯 종류만 맡고 남은 내구도는 호 길이로 보여 준다
			const float MaxDurability = bIsEquipped ? InState.Part->GetMaxDurability() : 0.0f;
			const float DurabilityRatio = MaxDurability > 0.0f ? InState.Durability / MaxDurability : 0.0f;

			ArcMaterial->SetScalarParameterValue(PercentParameterName, DurabilityRatio);
			ArcMaterial->SetVectorParameterValue(ColorParameterName, bIsEquipped ? SlotColor : EmptyColor);
		}
	}

	if (PartIcon)
	{
		UTexture2D* Icon = InState.GetIcon();
		if (bIsEquipped && Icon)
		{
			PartIcon->SetBrushFromTexture(Icon);
			PartIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			PartIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// 아이콘 색은 쿨다운 표시와 함께 정하므로 남은 쿨다운으로 다시 그린다
	const UWorld* World = GetWorld();
	UpdateAbilityVisuals((CooldownEndTime > 0.0f && World) ? CooldownEndTime - World->GetTimeSeconds() : 0.0f);

	if (PartNameText)
	{
		if (bInArmSlot && (!bIsEquipped || InState.Condition == EURGPartCondition::Broken))
		{
			PartNameText->SetText(bIsEquipped
				? FText::Format(BrokenArmNameFormat, InState.GetDisplayName(), MeleeAttackText)
				: MeleeAttackText);
			PartNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else if (bIsEquipped)
		{
			PartNameText->SetText(InState.GetDisplayName());
			PartNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			PartNameText->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UURGPartSlotUI::SetSynergyTags(const FText& InTagText, const FLinearColor& InColor) const
{
	if (SynergyTagText == nullptr)
	{
		return;
	}

	if (InTagText.IsEmpty())
	{
		SynergyTagText->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	SynergyTagText->SetText(InTagText);
	SynergyTagText->SetColorAndOpacity(FSlateColor(InColor));
	SynergyTagText->SetVisibility(ESlateVisibility::HitTestInvisible);
}


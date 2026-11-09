#include "UI/URGAmmoUI.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"

void UURGAmmoUI::SetAmmo(const int32 InCurrent, const int32 InMagazineSize)
{
	// 탄창이 없는 무기(근접)나 무기 미장착 시에는 표시하지 않는다
	if (InMagazineSize <= 0)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (AmmoText)
	{
		AmmoText->SetText(FText::Format(AmmoTextFormat, FText::AsNumber(InCurrent), FText::AsNumber(InMagazineSize)));
	}

	if (AmmoArc == nullptr)
	{
		return;
	}

	if (UMaterialInstanceDynamic* ArcMaterial = AmmoArc->GetDynamicMaterial())
	{
		static const FName PercentParameterName(TEXT("Percent"));
		ArcMaterial->SetScalarParameterValue(PercentParameterName, static_cast<float>(InCurrent) / InMagazineSize);
	}
}


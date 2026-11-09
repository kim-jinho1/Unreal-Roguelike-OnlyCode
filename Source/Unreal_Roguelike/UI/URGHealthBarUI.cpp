#include "UI/URGHealthBarUI.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"

void UURGHealthBarUI::SetRatio(const float InRatio) const
{
	if (HealthBar)
	{
		HealthBar->SetPercent(InRatio);
	}

	if (HealthArc == nullptr)
	{
		return;
	}

	// 브러시 머티리얼의 다이내믹 인스턴스는 첫 호출 때 한 번 만들어지고 이후 재사용된다
	if (UMaterialInstanceDynamic* ArcMaterial = HealthArc->GetDynamicMaterial())
	{
		static const FName PercentParameterName(TEXT("Percent"));
		static const FName ColorParameterName(TEXT("Color"));

		ArcMaterial->SetScalarParameterValue(PercentParameterName, InRatio);
		ArcMaterial->SetVectorParameterValue(ColorParameterName, InRatio < LowHealthRatio ? LowHealthColor : HealthColor);
	}
}

void UURGHealthBarUI::SetText(const float InCurrent, const float InMax) const
{
	if (HealthText == nullptr)
	{
		return;
	}

	// 소수점 없이 정수로 표시한다
	FNumberFormattingOptions Options;
	Options.SetMaximumFractionalDigits(0);

	HealthText->SetText(FText::Format(
		HealthTextFormat,
		FText::AsNumber(InCurrent, &Options),
		FText::AsNumber(InMax, &Options)));
}


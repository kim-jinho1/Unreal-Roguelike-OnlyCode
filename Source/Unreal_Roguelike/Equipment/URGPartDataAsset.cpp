#include "Equipment/URGPartDataAsset.h"

FPrimaryAssetId UURGPartDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("URGPart"), GetFName());
}


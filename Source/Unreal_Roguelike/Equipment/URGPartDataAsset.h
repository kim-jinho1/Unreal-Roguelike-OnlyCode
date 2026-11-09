#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "URGDefine.h"
#include "URGPartDataAsset.generated.h"

/**
 * 다리 부품이 주는 이동 능력. 대시는 기본 능력이 아니라 bCanDash인 다리 부품만 제공한다.
 * 배율은 캐릭터의 기본값(MoveSpeed·DashSpeed·DashCooldown)이 기준점이다.
 */
USTRUCT(BlueprintType)
struct FURGLegStats
{
	GENERATED_BODY()

	/** 대시 가능 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bCanDash = false;

	/** 이동 속도 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.1"))
	float MoveSpeedScale = 1.0f;

	/** 대시 속도 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.1", EditCondition = "bCanDash"))
	float DashSpeedScale = 1.0f;

	/** 대시 쿨다운 배율 (작을수록 자주 대시) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.1", EditCondition = "bCanDash"))
	float DashCooldownScale = 1.0f;

	/** 최대 점프 횟수 (2면 2단 점프) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 JumpMaxCount = 1;

};

/**
 * 머리 부품이 주는 패시브 수치. 치명타는 무기 데미지에, 드랍률은 드랍 시스템에, 적 표시는 HUD/연출에 쓴다.
 */
USTRUCT(BlueprintType)
struct FURGHeadStats
{
	GENERATED_BODY()

	/** 치명타 확률 (0~1) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "1"))
	float CritChance = 0.0f;

	/** 치명타 시 데미지 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	float CritDamageScale = 2.0f;

	/** 드랍률 배율 (드랍 시스템이 GetDropRateScale로 읽는다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float DropRateScale = 1.0f;

	/** 적 위치 표시 여부 (표시 연출은 미구현) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bRevealEnemies = false;

};

/**
 * 부품 하나의 정의 데이터.
 * 어느 슬롯에 들어가는지, HUD에 보일 이름/아이콘, 시너지 태그를 가진다.
 * 팔 슬롯 부품은 장착 시 스폰할 무기 클래스를, 다리는 이동·대시 배율을, 코어는 액티브 능력 클래스를, 머리는 패시브 수치를 함께 지정한다.
 */
UCLASS(BlueprintType)
class UNREAL_ROGUELIKE_API UURGPartDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	FORCEINLINE EURGPartSlot GetSlot() const { return Slot; }
	FORCEINLINE const FText& GetDisplayName() const { return DisplayName; }
	FORCEINLINE const FText& GetDescription() const { return Description; }
	FORCEINLINE UTexture2D* GetIcon() const { return Icon; }
	FORCEINLINE const FGameplayTagContainer& GetSynergyTags() const { return SynergyTags; }
	FORCEINLINE UStaticMesh* GetPickupMesh() const { return PickupMesh; }
	FORCEINLINE TSubclassOf<class AURGWeapon> GetWeaponClass() const { return WeaponClass; }
	FORCEINLINE const FURGLegStats& GetLegStats() const { return LegStats; }
	FORCEINLINE TSubclassOf<class UURGCoreAbility> GetCoreAbilityClass() const { return CoreAbilityClass; }
	FORCEINLINE const FURGHeadStats& GetHeadStats() const { return HeadStats; }
	FORCEINLINE float GetMaxDurability() const { return MaxDurability; }

protected:
	/** 이 부품이 들어가는 슬롯 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Info")
	EURGPartSlot Slot = EURGPartSlot::Arm;

	/** HUD 슬롯에 표시할 부품 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Info")
	FText DisplayName;

	/** 부품 설명 (획득/교체 UI용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Info", meta = (MultiLine = true))
	FText Description;

	/** HUD 슬롯 아이콘 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Info")
	TObjectPtr<UTexture2D> Icon;

	/** 시너지 태그 (Part.Tag.*). 같은 태그 2개 장착 시 효과 발동 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Synergy", meta = (Categories = "Part.Tag"))
	FGameplayTagContainer SynergyTags;

	/** 바닥에 떨어져 있을 때(픽업 액터) 보여줄 메시. 없으면 픽업 액터의 기본 메시 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Pickup")
	TObjectPtr<UStaticMesh> PickupMesh;

	/** 팔 슬롯 전용: 장착 시 스폰해 주 무기로 등록할 무기 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Arm", meta = (EditCondition = "Slot == EURGPartSlot::Arm", EditConditionHides))
	TSubclassOf<class AURGWeapon> WeaponClass;

	/** 다리 슬롯 전용: 이동·대시 배율과 점프 횟수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Leg", meta = (EditCondition = "Slot == EURGPartSlot::Leg", EditConditionHides))
	FURGLegStats LegStats;

	/** 코어 슬롯 전용: Q로 발동하는 액티브 능력 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Core", meta = (EditCondition = "Slot == EURGPartSlot::Core", EditConditionHides))
	TSubclassOf<class UURGCoreAbility> CoreAbilityClass;

	/** 머리 슬롯 전용: 치명타·드랍률·적 표시 패시브 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Head", meta = (EditCondition = "Slot == EURGPartSlot::Head", EditConditionHides))
	FURGHeadStats HeadStats;

	/** 최대 내구도 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part|Durability", meta = (ClampMin = "1"))
	float MaxDurability = 100.0f;

};


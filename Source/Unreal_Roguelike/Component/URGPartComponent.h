#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "URGDefine.h"
#include "URGPartComponent.generated.h"

class UTexture2D;
class UURGPartDataAsset;
struct FHitResult;

/** 부품 슬롯 하나의 상태 (장착 부품 + 손상 단계) */
USTRUCT(BlueprintType)
struct FURGPartSlotState
{
	GENERATED_BODY()

	/** 장착된 부품 데이터 (nullptr = 빈 슬롯) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PartSlot")
	TObjectPtr<UURGPartDataAsset> Part;

	/** 손상 단계 (장착 중일 때만 의미 있음) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PartSlot")
	EURGPartCondition Condition = EURGPartCondition::Normal;

	/** 현재 내구도 (장착 시 부품의 최대 내구도로 채움) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PartSlot")
	float Durability = 0.0f;

	FORCEINLINE bool IsEquipped() const { return Part != nullptr; }

	/** HUD 표시용 부품 이름 (빈 슬롯이면 빈 텍스트) */
	FText GetDisplayName() const;

	/** HUD 표시용 아이콘 (빈 슬롯이면 nullptr) */
	UTexture2D* GetIcon() const;

};

/** 슬롯 상태 변경 알림 (Slot, NewState) */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDelegateOnSlotChanged, EURGPartSlot, const FURGPartSlotState&);

/** 태그 시너지 발동·해제 알림 (SynergyTag, bActive) */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDelegateOnSynergyChanged, const FGameplayTag&, bool);

/**
 * 부품 슬롯 컴포넌트.
 * 4개 슬롯(팔/다리/코어/머리)의 장착 부품·손상 단계를 보유하고 변경을 델리게이트로 알린다.
 * 같은 태그 부품 수로 정해지는 시너지 발동 여부도 기록해, 켜지고 꺼진 순간만 OnSynergyChanged로 알린다.
 * 피격 뼈 → 슬롯 표로 맞은 부위의 슬롯을 찾아 준다.
 * 부품·시너지가 실제로 미치는 효과(무기 교체, 대시 값, 최대 체력 등)는 소유 캐릭터가 델리게이트를 받아 적용한다.
 * HUD는 같은 델리게이트로 표시만 갱신한다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREAL_ROGUELIKE_API UURGPartComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UURGPartComponent();

protected:
	virtual void BeginPlay() override;

public:
	/** 슬롯 상태 조회 (없는 슬롯이면 빈 상태) */
	const FURGPartSlotState& GetSlotState(EURGPartSlot InSlot) const;

	FORCEINLINE const TMap<EURGPartSlot, FURGPartSlotState>& GetSlots() const { return Slots; }

	/** 네 슬롯을 저장된 상태로 덮어쓰고 전부 변경을 알린다 (스테이지 이동 복원용) */
	void RestoreSlots(const TMap<EURGPartSlot, FURGPartSlotState>& InSlots);

	/** 슬롯을 사용할 수 있는지 (장착 중이고 파손되지 않음) */
	bool IsSlotUsable(EURGPartSlot InSlot) const;

	/** 손상 단계에 따른 효과 배율 (정상 1, 손상 DamagedScale, 파손·빈 슬롯 0) */
	float GetConditionScale(EURGPartSlot InSlot) const;

	/** 부품 장착: 부품이 지정한 슬롯에 정상 상태로 넣는다 (기존 부품은 교체) */
	void EquipPart(UURGPartDataAsset* InPart);

	/** 슬롯 비우기 (적 AI 자동화 테스트가 쓴다) */
	void UnequipPart(EURGPartSlot InSlot);

	/** 슬롯 내구도 차감, 임계치를 지나면 Damaged/Broken으로 전이 (빈 슬롯·파손 슬롯은 무시) */
	void ApplyPartDamage(EURGPartSlot InSlot, float InDamage);

	/** 슬롯 내구도 회복, 회복 후 비율로 단계를 다시 정한다 (빈 슬롯은 무시) */
	void RepairSlot(EURGPartSlot InSlot, float InAmount);

	/** 수리가 필요한지 (장착 중이고 내구도가 최대 미만) */
	bool NeedsRepair(EURGPartSlot InSlot) const;

	/** 슬롯 손상 단계 변경 (빈 슬롯이면 무시, 적 AI 자동화 테스트가 쓴다) */
	void SetSlotCondition(EURGPartSlot InSlot, EURGPartCondition InCondition);

	/** 사용 가능한 슬롯(장착 중·파손 아님) 중 이 시너지 태그를 가진 부품 수 */
	int32 GetSynergyTagCount(const FGameplayTag& InTag) const;

	/** 시너지 발동에 필요한 같은 태그 부품 수 (HUD 진행도 표시용) */
	FORCEINLINE int32 GetSynergyThreshold() const { return SynergyThreshold; }

	/** 지금 발동 중인 시너지 태그 */
	FORCEINLINE const FGameplayTagContainer& GetActiveSynergies() const { return ActiveSynergies; }

	/** 시너지 발동 여부 (태그 수가 SynergyThreshold 이상) */
	FORCEINLINE bool IsSynergyActive(const FGameplayTag& InTag) const { return ActiveSynergies.HasTagExact(InTag); }

	/** InPart를 제 슬롯에 새로 장착했다고 가정할 때의 시너지 발동 여부 (부품 비교 UI 미리보기용) */
	bool IsSynergyActiveWithPart(const FGameplayTag& InTag, const UURGPartDataAsset* InPart) const;

	/** 부품 내구도 차감에 추가로 곱할 배율 (강화 시너지가 조정) */
	FORCEINLINE void SetPartDamageScale(float InScale) { PartDamageScale = FMath::Max(0.0f, InScale); }

	/**
	 * 피격 결과의 뼈(없으면 충돌점에서 가장 가까운 바디 뼈)에서 부모로 올라가며 HitBoneSlotMap에 걸리는 슬롯을 찾는다.
	 * InMesh는 피격자의 스켈레탈 메시 (소유자 타입을 몰라도 되게 호출자가 넘긴다). 못 찾으면 false
	 */
	bool ResolveSlotFromHit(const FHitResult& InHit, const class USkeletalMeshComponent* InMesh, EURGPartSlot& OutSlot) const;

public:
	/** 슬롯 상태가 변경될 때 호출 */
	FDelegateOnSlotChanged OnSlotChanged;

	/** 시너지가 켜지거나 꺼질 때 호출 (OnSlotChanged 다음, 바뀐 태그만) */
	FDelegateOnSynergyChanged OnSynergyChanged;

protected:
	/** 시작 시 장착할 기본 부품 (기획서 초기 상태: 팔·다리) */
	UPROPERTY(EditAnywhere, Category = "Part|Default")
	TArray<TObjectPtr<UURGPartDataAsset>> DefaultParts;

	/** 손상(Damaged) 단계의 효과 배율 */
	UPROPERTY(EditAnywhere, Category = "Part|Condition", meta = (ClampMin = "0", ClampMax = "1"))
	float DamagedScale = 0.7f;

	/** 체력 데미지 대비 내구도 차감 배율 (1이면 그대로, 0이면 부품 손상 없음) */
	UPROPERTY(EditAnywhere, Category = "Part|Condition", meta = (ClampMin = "0"))
	float PartDamageRatio = 1.0f;

	/** 내구도가 최대치의 이 비율 이하로 떨어지면 Damaged */
	UPROPERTY(EditAnywhere, Category = "Part|Condition", meta = (ClampMin = "0", ClampMax = "1"))
	float DamagedThreshold = 0.5f;

	/** 같은 시너지 태그가 이 개수 이상 모이면 발동 */
	UPROPERTY(EditAnywhere, Category = "Part|Synergy", meta = (ClampMin = "1"))
	int32 SynergyThreshold = 2;

	/** 피격 뼈 → 부품 슬롯. 부위 루트 뼈만 등록하면 하위 뼈는 부모 탐색으로 흡수된다. 어느 부위에도 안 걸리면 부품 손상 없음 */
	UPROPERTY(EditAnywhere, Category = "Part|Damage")
	TMap<FName, EURGPartSlot> HitBoneSlotMap;

	/** 슬롯별 상태 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Part|Slot")
	TMap<EURGPartSlot, FURGPartSlotState> Slots;

private:
	/** 시너지 기록을 갱신한 뒤 슬롯 변경, 이어서 바뀐 시너지만 알린다 (슬롯 핸들러가 최신 시너지 기록을 읽도록 이 순서) */
	void NotifySlotChanged(EURGPartSlot InSlot);

	/**
	 * 사용 가능한 슬롯의 부품 중 이 태그를 가진 수.
	 * InReplacePart가 있으면 그 부품이 제 슬롯에 정상 상태로 들어왔다고 가정하고 센다 (비교 UI 미리보기)
	 */
	int32 CountSynergyTag(const FGameplayTag& InTag, const UURGPartDataAsset* InReplacePart) const;

private:
	/** 시너지 등 런타임 효과가 정한 내구도 차감 배율 (PartDamageRatio에 추가로 곱한다) */
	float PartDamageScale = 1.0f;

	/** 발동 중인 시너지 태그 (켜지고 꺼진 순간만 알리기 위해 기록) */
	FGameplayTagContainer ActiveSynergies;

};


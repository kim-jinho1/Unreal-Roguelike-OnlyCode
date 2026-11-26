#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/URGInteractable.h"
#include "URGDefine.h"
#include "URGRepairStation.generated.h"

/** 슬롯 하나의 수리 규칙: 어떤 미니게임으로, 몇 점 이상이면 성공인지 */
USTRUCT(BlueprintType)
struct FURGRepairRule
{
	GENERATED_BODY()

	FURGRepairRule() = default;
	FURGRepairRule(TSubclassOf<class UURGMinigameUI> InMinigameClass, float InRequiredScore)
		: MinigameClass(InMinigameClass), RequiredScore(InRequiredScore)
	{
	}

	/** 이 슬롯을 고칠 때 여는 미니게임 위젯 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<class UURGMinigameUI> MinigameClass;

	/** 수리 성공으로 인정하는 최소 점수 (1이면 전부 완료해야 수리) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "1"))
	float RequiredScore = 1.0f;

};

/**
 * 맵에 놓이는 수리 장치. 상호작용(E)하면 슬롯 선택 메뉴 → 그 슬롯의 미니게임 순으로 열고,
 * 미니게임 점수(0~1)가 슬롯 규칙의 RequiredScore 이상이면 그 슬롯을 완전히 수리하고, 미만이면 회복 없이 닫는다.
 * 메뉴·미니게임이 열려 있는 동안의 입력 모드 전환은 플레이어 컨트롤러에 맡기고, 어느 경로로 닫히든 CloseSession 한 곳에서 정리한다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API AURGRepairStation : public AActor, public IURGInteractable
{
	GENERATED_BODY()

public:
	AURGRepairStation();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** IURGInteractable */
	virtual void Interact_Implementation(AActor* InInstigator) override;
	virtual FText GetInteractText_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* InInstigator) const override;

protected:
	/** 메뉴에서 슬롯을 골랐을 때: 슬롯 규칙의 미니게임을 만들고 비용 차감 후 시작 */
	void HandleSlotSelected(EURGPartSlot InSlot);

	/** 메뉴를 닫았을 때 */
	void HandleMenuCancelled();

	/** 미니게임 종료: 점수가 슬롯 기준 이상이면 완전 수리 */
	void HandleMinigameFinished(float InScore);

	/** 세션 중 플레이어 사망: 세션을 닫는다 (입력 모드는 컨트롤러가 사망 상태를 보고 지킨다) */
	void HandleInstigatorDeath();

	/** 위젯 정리 + 모달 닫기 + 사망 바인딩 해제 + 세션 참조 비우기 */
	void CloseSession();

protected:
	/** 장치 외형. 상호작용 트레이스에만 걸린다 */
	UPROPERTY(VisibleAnywhere, Category = "RepairStation|Mesh")
	TObjectPtr<class UStaticMeshComponent> Mesh;

	/** 슬롯 선택 메뉴 위젯 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "RepairStation|UI")
	TSubclassOf<class UURGRepairMenuUI> MenuClass;

	/** 슬롯별 수리 미니게임과 성공 기준. 규칙이 없거나 미니게임 클래스가 빈 슬롯을 고르면 비용 없이 세션을 닫는다 */
	UPROPERTY(EditDefaultsOnly, Category = "RepairStation|Rule")
	TMap<EURGPartSlot, FURGRepairRule> SlotRules;

	/** 수리 1회 비용 (0이면 무료) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RepairStation|Cost", meta = (ClampMin = "0"))
	int32 RepairCost = 30;

	/** 수리 성공 시 함께 회복하는 체력 (0이면 회복 없음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RepairStation|Rule", meta = (ClampMin = "0"))
	float RepairHealAmount = 30.0f;

	/** 상호작용 안내 문구 */
	UPROPERTY(EditDefaultsOnly, Category = "RepairStation|Text")
	FText InteractText = NSLOCTEXT("URG.RepairStation", "Interact", "수리");

private:
	UPROPERTY()
	TObjectPtr<class UURGRepairMenuUI> Menu;

	UPROPERTY()
	TObjectPtr<class UURGMinigameUI> Minigame;

	/** 세션을 연 플레이어 컨트롤러 (모달 입력 모드 전환용) */
	TWeakObjectPtr<class AURGPlayerController> PlayerController;

	/** 세션을 연 플레이어의 부품 컴포넌트 */
	TWeakObjectPtr<class UURGPartComponent> PartComponent;

	/** 세션을 연 플레이어의 자원 컴포넌트 (비용 차감용) */
	TWeakObjectPtr<class UURGResourceComponent> ResourceComponent;

	/** 세션을 연 플레이어의 속성 컴포넌트 (사망 감지 + 수리 성공 시 체력 회복) */
	TWeakObjectPtr<class UURGAttributeComponent> AttributeComponent;

	/** OnDeath 바인딩 해제용 핸들 */
	FDelegateHandle DeathHandle;

	/** 수리 중인 슬롯 */
	EURGPartSlot TargetSlot = EURGPartSlot::Arm;

	/** 수리 중인 슬롯의 성공 기준 점수 (슬롯을 고를 때 규칙에서 옮겨 둔다) */
	float TargetRequiredScore = 1.0f;

	/** 메뉴 또는 미니게임이 열려 있는지 */
	bool bIsSessionOpen = false;

};

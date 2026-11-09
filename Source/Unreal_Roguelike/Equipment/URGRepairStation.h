#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/URGInteractable.h"
#include "URGDefine.h"
#include "URGRepairStation.generated.h"

/**
 * 맵에 놓이는 수리 장치. 상호작용(E)하면 슬롯 선택 메뉴 → 미니게임 순으로 열고,
 * 미니게임 점수(0~1)가 RequiredScore 이상이면 그 슬롯을 완전히 수리하고, 미만이면 회복 없이 닫는다.
 * 메뉴·미니게임이 열려 있는 동안 입력 모드를 UI로 바꾸고, 어느 경로로 닫히든 CloseSession에서 게임 입력으로 되돌린다.
 * 세션 중 플레이어가 죽으면 위젯만 치우고 입력 모드는 건드리지 않는다 (게임오버 입력 모드는 컨트롤러가 정한다).
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
	/** 메뉴에서 슬롯을 골랐을 때: 비용 차감 후 미니게임 시작 */
	void HandleSlotSelected(EURGPartSlot InSlot);

	/** 메뉴를 닫았을 때 */
	void HandleMenuCancelled();

	/** 미니게임 종료: 점수가 RequiredScore 이상이면 완전 수리 */
	void HandleMinigameFinished(float InScore);

	/** 세션 중 플레이어 사망: 위젯만 정리 */
	void HandleInstigatorDeath();

	/** 위젯 정리 + 사망 바인딩 해제. bInRestoreInput이면 게임 입력 모드로 복구 (사망 시엔 false) */
	void CloseSession(bool bInRestoreInput = true);

	/** 입력 모드를 UI로 바꾸고 위젯에 포커스 */
	void FocusWidget(class UUserWidget* InWidget) const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "RepairStation|Mesh")
	TObjectPtr<class UStaticMeshComponent> Mesh;

	/** 슬롯 선택 메뉴 위젯 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "RepairStation|UI")
	TSubclassOf<class UURGRepairMenuUI> MenuClass;

	/** 수리 미니게임 위젯 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "RepairStation|UI")
	TSubclassOf<class UURGMinigameUI> MinigameClass;

	/** 다리 전용 용접. 기본 C++ 위젯은 별도 WBP 없이 동작한다. */
	UPROPERTY(EditDefaultsOnly, Category = "RepairStation|UI")
	TSubclassOf<class UURGWeldMinigameUI> LegMinigameClass;

	/** 코어 전용 배선 연결. 기본 C++ 위젯은 별도 WBP 없이 동작한다. */
	UPROPERTY(EditDefaultsOnly, Category = "RepairStation|UI")
	TSubclassOf<class UURGWireMinigameUI> CoreMinigameClass;

	/** 머리 전용 램 장착. 파손 없이 장착 완료 시에만 수리한다. */
	UPROPERTY(EditDefaultsOnly, Category = "RepairStation|UI")
	TSubclassOf<class UURGRAMMinigameUI> HeadMinigameClass;

	/** 경로 이탈 거리를 평가하는 다리 용접의 성공 기준 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RepairStation|Rule", meta = (ClampMin = "0", ClampMax = "1"))
	float LegRequiredScore = 0.75f;

	/** 수리 1회 비용 (0이면 무료) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RepairStation|Cost", meta = (ClampMin = "0"))
	int32 RepairCost = 30;

	/** 수리 성공으로 인정하는 최소 점수 (1이면 전부 완료해야 수리) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RepairStation|Rule", meta = (ClampMin = "0", ClampMax = "1"))
	float RequiredScore = 1.0f;

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

	/** 세션을 연 플레이어 컨트롤러 */
	TWeakObjectPtr<class APlayerController> PlayerController;

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

	/** 메뉴 또는 미니게임이 열려 있는지 */
	bool bIsSessionOpen = false;

};

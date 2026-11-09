#pragma once

#include "CoreMinimal.h"
#include "UI/URGMinigameUI.h"
#include "URGBoltMinigameUI.generated.h"

/**
 * 볼트 조이기 미니게임. 가운데 큰 볼트 하나를 BoltCount번 조인다.
 * 볼트 위에서 마우스를 누르고 시계 방향으로 원을 그리면 각도가 쌓이고, RequiredTurns 바퀴를 채우면 한 개를 조인 것으로 치고
 * 볼트를 처음 상태로 돌려 다음 개로 넘어간다. 볼트 둘레 링은 지금 볼트의 진행도, 아래 점은 조인 개수.
 * 전부 조이면 점수 1, 포기·시간 초과면 조인 비율.
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGBoltMinigameUI : public UURGMinigameUI
{
	GENERATED_BODY()

public:
	virtual void Start() override;

protected:
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	/** 화면 좌표가 볼트 잡기 반경 안인지 */
	bool IsOverBolt(const FVector2D& InScreenPosition) const;

	/** 볼트 중심 기준 마우스 각도 (도, 시계 방향 증가) */
	float GetAngleAround(const FVector2D& InScreenPosition) const;

	/** 볼트 회전·색과 둘레 링 갱신 */
	void UpdateBoltVisual() const;

	/** 진행 점 갱신 */
	void UpdateProgressDots() const;

	/** 렌치 이미지를 마우스 위치로 옮긴다 */
	void MoveWrench(const FGeometry& InGeometry, const FVector2D& InScreenPosition, float InAngle) const;

	/** 조인 비율 (0~1) */
	float GetProgress() const;

	/** 시간 초과 (타이머 콜백): 현재 진행도로 종료 */
	void HandleTimeUp();

protected:
	/** 조일 볼트 (한 개를 반복해서 쓴다) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UImage> BoltImage;

	/** 볼트 둘레 진행 링 (호 머티리얼, 없어도 동작) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UImage> BoltArc;

	/** 자식 Image 하나하나가 진행 점 (없어도 동작). BoltCount개만 쓰고 나머지는 숨긴다 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UPanelWidget> ProgressDots;

	/** 마우스를 따라다니는 렌치 (없어도 동작) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UImage> WrenchImage;

	/** 조여야 하는 볼트 개수 */
	UPROPERTY(EditDefaultsOnly, Category = "BoltMinigameUI|Rule", meta = (ClampMin = "1"))
	int32 BoltCount = 4;

	/** 볼트 하나를 조이는 데 필요한 바퀴 수 */
	UPROPERTY(EditDefaultsOnly, Category = "BoltMinigameUI|Rule", meta = (ClampMin = "0.25"))
	float RequiredTurns = 2.0f;

	/** 잡기 반경 = 볼트 이미지 반지름 × 이 값 */
	UPROPERTY(EditDefaultsOnly, Category = "BoltMinigameUI|Rule", meta = (ClampMin = "1"))
	float GrabRadiusScale = 1.5f;

	/** 제한 시간 (초). 0이면 무제한 */
	UPROPERTY(EditDefaultsOnly, Category = "BoltMinigameUI|Rule", meta = (ClampMin = "0"))
	float TimeLimit = 0.0f;

	/** 안 조인 볼트 색 */
	UPROPERTY(EditDefaultsOnly, Category = "BoltMinigameUI|Color")
	FLinearColor LooseColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);

	/** 다 조인 볼트 색 */
	UPROPERTY(EditDefaultsOnly, Category = "BoltMinigameUI|Color")
	FLinearColor TightColor = FLinearColor::White;

	/** 진행 링·채운 점 색 */
	UPROPERTY(EditDefaultsOnly, Category = "BoltMinigameUI|Color")
	FLinearColor ProgressColor = FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);

	/** 아직 안 조인 점 색 */
	UPROPERTY(EditDefaultsOnly, Category = "BoltMinigameUI|Color")
	FLinearColor DotEmptyColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.2f);

private:
	/** 지금까지 조인 개수 */
	int32 TightenedCount = 0;

	/** 지금 볼트를 돌린 누적 각도 */
	float TurnedDegrees = 0.0f;

	/** 볼트를 잡고 있는지 */
	bool bIsHolding = false;

	/** 직전 마우스 각도 (누적 계산용) */
	float LastAngle = 0.0f;

	FTimerHandle TimeLimitTimer;

};

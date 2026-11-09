#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "URGEnemyRadarUI.generated.h"

/**
 * 적 방향 마커 UI (머리 부품 "적 탐지 레이더").
 * 화면 중앙을 둘러싼 원 위에 적 방향마다 마커를 하나씩 놓는다. 위쪽이 카메라 정면.
 * 적 위치는 레이더 컴포넌트가 주기적으로 넘겨주고(SetTargets), 카메라가 돌 때 마커가 따라 돌도록
 * 대상이 있을 때만 NativeTick에서 마커 각도를 다시 계산한다 (데이터 조회는 하지 않는다).
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGEnemyRadarUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	/** 표시할 적 위치 목록 반영 (비어 있으면 마커를 모두 숨긴다) */
	void SetTargets(const TArray<FVector>& InTargets);

private:
	/** 필요한 수만큼 마커 이미지를 만들어 둔다 (한 번 만든 마커는 재사용) */
	void EnsureMarkers(int32 InCount);

	/** 카메라 정면 기준으로 각 대상의 방향을 원 위 위치·회전으로 바꿔 마커에 반영 */
	void UpdateMarkers();

protected:
	/** 마커를 올릴 캔버스 (화면 전체) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UCanvasPanel> MarkerCanvas;

private:
	/** 화면 중앙에서 마커까지 거리 (px) */
	UPROPERTY(EditDefaultsOnly, Category = "EnemyRadarUI|Marker")
	float MarkerRadius = 200.0f;

	/** 마커 크기 (px) */
	UPROPERTY(EditDefaultsOnly, Category = "EnemyRadarUI|Marker")
	FVector2D MarkerSize = FVector2D(14.0f, 22.0f);

	/** 마커 색 */
	UPROPERTY(EditDefaultsOnly, Category = "EnemyRadarUI|Marker")
	FLinearColor MarkerColor = FLinearColor(1.0f, 0.25f, 0.2f, 0.9f);

	/** 마커 텍스처 (없으면 단색 사각형). 위쪽을 가리키는 화살표 모양이어야 회전이 맞는다 */
	UPROPERTY(EditDefaultsOnly, Category = "EnemyRadarUI|Marker")
	TObjectPtr<class UTexture2D> MarkerTexture;

	/** 만들어 둔 마커 (재사용 풀) */
	UPROPERTY(Transient)
	TArray<TObjectPtr<class UImage>> Markers;

	/** 마지막으로 받은 적 위치 */
	TArray<FVector> Targets;

};


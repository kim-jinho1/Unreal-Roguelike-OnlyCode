#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "URGDefine.h"
#include "URGGameOverUI.generated.h"

/** 다시 시작 요청 알림 */
DECLARE_MULTICAST_DELEGATE(FDelegateOnRetryRequested);

/** 게임 종료 요청 알림 */
DECLARE_MULTICAST_DELEGATE(FDelegateOnQuitRequested);

/** 타이틀로 돌아가기 요청 알림 */
DECLARE_MULTICAST_DELEGATE(FDelegateOnTitleRequested);

/**
 * 런이 끝났을 때 뜨는 결과 화면 (게임오버·엔딩 공용). 도달한 스테이지·방과 마지막 부품 슬롯 4개 상태를 보여 주고 다시 시작·타이틀·종료를 고르게 한다.
 * 부품 슬롯은 HUD와 같은 슬롯 위젯을 써서 파손·내구도가 같은 모양으로 보인다. 게임오버와 엔딩은 문구만 다른 WBP로 나눈다.
 * 실제 재시작·종료는 하지 않고 델리게이트로 컨트롤러에 알린다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API UURGGameOverUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

public:
	/** 도달 진행도 텍스트 갱신 */
	void SetProgress(int32 InChapter, int32 InRoom, int32 InTotalRooms) const;

	/** 쓰러질 때의 부품 슬롯 상태 반영 (부품 컴포넌트가 없으면 아무것도 하지 않는다) */
	void SetPartSlots(const class UURGPartComponent* InPartComponent) const;

public:
	/** 다시 시작 버튼을 눌렀을 때 호출 */
	FDelegateOnRetryRequested OnRetryRequested;

	/** 종료 버튼을 눌렀을 때 호출 */
	FDelegateOnQuitRequested OnQuitRequested;

	/** 타이틀로 버튼을 눌렀을 때 호출 */
	FDelegateOnTitleRequested OnTitleRequested;

protected:
	UFUNCTION()
	void HandleRetryClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleTitleClicked();

private:
	/** 슬롯 하나의 위젯·설명 글자 갱신 */
	void SetupSlot(class UURGPartSlotUI* InSlotUI, class UTextBlock* InText, const UURGPartComponent* InPartComponent, EURGPartSlot InSlot) const;

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UButton> RetryButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UButton> QuitButton;

	/** 타이틀로 돌아가기 (없는 WBP는 버튼을 표시하지 않는다) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UButton> TitleButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> ProgressText;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UURGPartSlotUI> ArmSlot;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UURGPartSlotUI> LegSlot;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UURGPartSlotUI> CoreSlot;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UURGPartSlotUI> HeadSlot;

	/** 슬롯 아래 설명 (부품 이름 + 내구도 또는 파손) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> ArmText;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> LegText;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> CoreText;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> HeadText;

private:
	/** 진행도 텍스트 포맷 ({0} = 스테이지, {1} = 방, {2} = 전체 방 수) */
	UPROPERTY(EditDefaultsOnly, Category = "GameOverUI|Text")
	FText ProgressTextFormat = NSLOCTEXT("URG.GameOverUI", "ProgressFormat", "STAGE {0}  ·  {1}/{2}");

	/** 장착 중인 슬롯 설명 ({0} = 부품 이름, {1} = 내구도 %) */
	UPROPERTY(EditDefaultsOnly, Category = "GameOverUI|Text")
	FText SlotTextFormat = NSLOCTEXT("URG.GameOverUI", "SlotFormat", "{0} {1}%");

	/** 파손된 슬롯 설명 ({0} = 부품 이름) */
	UPROPERTY(EditDefaultsOnly, Category = "GameOverUI|Text")
	FText BrokenTextFormat = NSLOCTEXT("URG.GameOverUI", "BrokenFormat", "{0} 파손");

	/** 빈 슬롯 설명 */
	UPROPERTY(EditDefaultsOnly, Category = "GameOverUI|Text")
	FText EmptySlotText = NSLOCTEXT("URG.GameOverUI", "EmptySlot", "빈 슬롯");

	/** 파손된 슬롯 설명 글자 색 */
	UPROPERTY(EditDefaultsOnly, Category = "GameOverUI|Color")
	FLinearColor BrokenTextColor = FLinearColor(1.0f, 0.25f, 0.2f, 1.0f);

	/** 그 밖의 슬롯 설명 글자 색 */
	UPROPERTY(EditDefaultsOnly, Category = "GameOverUI|Color")
	FLinearColor NormalTextColor = FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);

};

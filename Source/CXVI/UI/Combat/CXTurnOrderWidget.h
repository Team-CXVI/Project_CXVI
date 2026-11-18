// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "CXTurnOrderTypes.h"

#include "CXTurnOrderWidget.generated.h"

class UCanvasPanel;
class UCXTurnOrderEntryWidget;

/**
 * 전투 턴 순서 목록 (WBP_TurnOrder의 부모)
 * - WBP_CombatHUD의 View Binding : CXCombatViewModel.TurnOrder -> SetTurnOrder
 * - 이전 목록과 새 목록을 순서 유지 비교(LCS)해서 유지 / 퇴장 / 등장으로 나눔
 *   -> 같은 캐릭터가 여러 칸에 나올 수 있어 캐릭터 포인터 조회만으로는 칸을 구분할 수 없음
 * - 유지된 칸은 새 위치로 이동, 빠진 칸은 ExitAnim 후 제거, 새 칸은 EnterAnim
 * - 목록 맨 앞(현재 턴) 칸을 하이라이트
 * - 연출 중에 다시 갱신되면 현재 위치에서 새 목표로 이어서 이동
 */
UCLASS(Abstract)
class CXVI_API UCXTurnOrderWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "CXVI|Combat")
	void SetTurnOrder(const TArray<FCXTurnOrderEntryData>& InEntries);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// 결과 : 이전 목록 인덱스 -> 새 목록 인덱스 (빠지는 칸은 INDEX_NONE)
	TArray<int32> BuildRetainedMap(const TArray<FCXTurnOrderEntryData>& InEntries) const;

	UCXTurnOrderEntryWidget* CreateEntry(const FCXTurnOrderEntryData& InData);

	void HandleEntryExited(UCXTurnOrderEntryWidget* InEntry);

	FVector2D GetRowPosition(int32 InIndex) const;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> EntryCanvas;

	UPROPERTY(EditAnywhere, Category = "Turn Order")
	TSubclassOf<UCXTurnOrderEntryWidget> EntryClass;

	// 맨 앞 칸의 위치
	UPROPERTY(EditAnywhere, Category = "Turn Order")
	FVector2D FirstRowPosition = FVector2D::ZeroVector;

	// 칸 사이 간격. X를 주면 계단형 배치
	UPROPERTY(EditAnywhere, Category = "Turn Order")
	FVector2D RowStep = FVector2D(-8.f, 96.f);

	UPROPERTY(EditAnywhere, Category = "Turn Order", meta = (ClampMin = "0.0"))
	float MoveDuration = 0.25f;

	// 목록이 비어 있다가 처음 채워질 때 칸마다 늦출 시간
	UPROPERTY(EditAnywhere, Category = "Turn Order", meta = (ClampMin = "0.0"))
	float EnterStaggerDelay = 0.05f;

	// 표시 순서대로 저장
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCXTurnOrderEntryWidget>> ActiveEntries;

	// ExitAnim 재생 중인 칸
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCXTurnOrderEntryWidget>> ExitingEntries;

	bool bHasMovingEntries = false;
	
};

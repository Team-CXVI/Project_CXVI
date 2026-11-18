// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Foundation/Combat/CombatMessage/CXCombatMessageData.h"

#include "CXDamageNumberWidget.generated.h"

class UTextBlock;
class UWidgetAnimation;

/**
 * 데미지 숫자 하나 (UCXDamageNumberStackWidget이 생성해서 Overlay에 추가)
 * - ShowResult : 결과에 맞는 텍스트를 넣고 결과별 애니메이션 재생
 *   - Hit / Critical : 피해량 (반올림)
 *   - Dodged / Parried : ResultTexts의 텍스트. 비어 있으면 표시하지 않음
 * - 결과별 애니메이션이 없으면 HitAnim으로 재생
 * - 재생 중인 애니메이션이 끝나면 RemoveFromParent
 * - 위치 / 크기 / 색 연출은 WBP 애니메이션 담당. 애니메이션은 루트가 아닌 DamageText에 걸 것
 */
UCLASS(Abstract)
class CXVI_API UCXDamageNumberWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	// 표시할 내용이 없으면 false (호출한 쪽에서 제거)
	bool ShowResult(ECXDamageResult InResult, float InAmount);

protected:
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* InAnimation) override;

private:
	bool BuildResultText(ECXDamageResult InResult, float InAmount, FText& OutText) const;

	UWidgetAnimation* FindResultAnimation(ECXDamageResult InResult) const;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DamageText;

	// 기본 애니메이션 (필수). 결과별 애니메이션이 없을 때도 사용
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> HitAnim;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> CriticalAnim;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> DodgedAnim;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> ParriedAnim;

	// 숫자 대신 표시할 텍스트 (Dodged / Parried). 항목이 없거나 비어 있으면 그 결과는 표시하지 않음
	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	TMap<ECXDamageResult, FText> ResultTexts;

	// 지금 재생 중인 애니메이션 (끝나면 제거)
	UPROPERTY(Transient)
	TObjectPtr<UWidgetAnimation> PlayingAnim;
	
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXDamageNumberWidget.h"

#include "UI/CXUIManagerSubsystem.h"

#include <Components/TextBlock.h>
#include <Animation/WidgetAnimation.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXDamageNumberWidget)

bool UCXDamageNumberWidget::ShowResult(ECXDamageResult InResult, float InAmount)
{
	FText ResultText;
	if (!IsValid(DamageText) || !BuildResultText(InResult, InAmount, ResultText))
	{
		return false;
	}

	DamageText->SetText(ResultText);

	PlayingAnim = FindResultAnimation(InResult);
	if (!IsValid(PlayingAnim))
	{
		UE_LOG(LogCXUI, Warning, TEXT("[DamageNumber] 재생할 애니메이션 없음 (%s의 HitAnim 확인)"), *GetNameSafe(GetClass()));
		return false;
	}

	PlayAnimation(PlayingAnim);
	return true;
}

void UCXDamageNumberWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* InAnimation)
{
	Super::OnAnimationFinished_Implementation(InAnimation);

	if (InAnimation == PlayingAnim)
	{
		PlayingAnim = nullptr;
		RemoveFromParent();
	}
}

bool UCXDamageNumberWidget::BuildResultText(ECXDamageResult InResult, float InAmount, FText& OutText) const
{
	switch (InResult)
	{
	case ECXDamageResult::Hit:
	case ECXDamageResult::Critical:
		// HP UI(UCXCombatantViewModel)와 같은 반올림
		OutText = FText::AsNumber(FMath::RoundToInt(InAmount));
		return true;

	default:
		if (const FText* Found = ResultTexts.Find(InResult))
		{
			OutText = *Found;
			return !OutText.IsEmpty();
		}
		return false;
	}
}

UWidgetAnimation* UCXDamageNumberWidget::FindResultAnimation(ECXDamageResult InResult) const
{
	UWidgetAnimation* ResultAnim = nullptr;

	switch (InResult)
	{
	case ECXDamageResult::Critical:	ResultAnim = CriticalAnim;	break;
	case ECXDamageResult::Dodged:	ResultAnim = DodgedAnim;	break;
	case ECXDamageResult::Parried:	ResultAnim = ParriedAnim;	break;
	default:						break;
	}

	return IsValid(ResultAnim) ? ResultAnim : HitAnim.Get();
}
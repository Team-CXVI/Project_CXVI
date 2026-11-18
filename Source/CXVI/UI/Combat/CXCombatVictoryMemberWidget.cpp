// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXCombatVictoryMemberWidget.h"

#include <CommonTextBlock.h>
#include <Components/Image.h>
#include <Components/ProgressBar.h>
#include <Engine/Texture2D.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatVictoryMemberWidget)

void UCXCombatVictoryMemberWidget::SetMember(TSoftObjectPtr<UTexture2D> InPortrait, int32 InLevel, bool bInLeveledUp, float InHealthPercent)
{
	// 지정되지 않은 경우 비워 둠
	if (InPortrait.IsNull())
	{
		PortraitImage->SetBrushResourceObject(nullptr);
	}
	else
	{
		PortraitImage->SetBrushFromSoftTexture(InPortrait);
	}

	LevelText->SetText(FText::FormatNamed(LevelFormat, TEXT("Level"), FText::AsNumber(InLevel)));

	HPBar->SetPercent(FMath::Clamp(InHealthPercent, 0.f, 1.f));

	LevelUpMark->SetVisibility(bInLeveledUp ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

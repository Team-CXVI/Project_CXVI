// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXCombatReactionHintWidget.h"

#include "Foundation/Combat/CombatMessage/CXCombatMessageData.h"
#include "Tags/CXGameplayTags.h"
#include "UI/CXInputHintWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatReactionHintWidget)

void UCXCombatReactionHintWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 메시지를 받기 전까지 숨김
	SetHintVisible(DodgeHint, false);
	SetHintVisible(ParryHint, false);

	ReactionAvailabilityListener = UCXGameplayMessageSubsystem::Get(this).RegisterListener(CXVIGameplayTags::Message_Combat_ReactionAvailabilityChanged, this, &ThisClass::HandleReactionAvailabilityChanged);
}

void UCXCombatReactionHintWidget::NativeDestruct()
{
	ReactionAvailabilityListener.Unregister();

	Super::NativeDestruct();
}

void UCXCombatReactionHintWidget::HandleReactionAvailabilityChanged(const FGameplayTag& InChannel, const FCXCombatReactionAvailabilityMessage& InMessage)
{
	SetHintVisible(DodgeHint, InMessage.bDodgeAvailable);
	SetHintVisible(ParryHint, InMessage.bParryAvailable);
}

void UCXCombatReactionHintWidget::SetHintVisible(UCXInputHintWidget* InHint, bool bInVisible)
{
	if (InHint)
	{
		InHint->SetVisibility(bInVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

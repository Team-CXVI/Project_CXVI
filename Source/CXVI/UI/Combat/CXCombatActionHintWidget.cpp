// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXCombatActionHintWidget.h"

#include "Foundation/Combat/CombatMessage/CXCombatMessageData.h"
#include "Tags/CXGameplayTags.h"
#include "UI/CXHoldActionWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatActionHintWidget)

void UCXCombatActionHintWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CommandAvailabilityListener = UCXGameplayMessageSubsystem::Get(this).RegisterListener(CXVIGameplayTags::Message_Combat_CommandAvailabilityChanged, this, &ThisClass::HandleCommandAvailabilityChanged);
}

void UCXCombatActionHintWidget::NativeDestruct()
{
	CommandAvailabilityListener.Unregister();

	Super::NativeDestruct();
}

void UCXCombatActionHintWidget::HandleCommandAvailabilityChanged(const FGameplayTag& InChannel, const FCXCombatCommandAvailabilityMessage& InMessage)
{
	SkipTurnHold->SetHoldActionActive(InMessage.bAvailable);
	FleeHold->SetHoldActionActive(InMessage.bAvailable);
}

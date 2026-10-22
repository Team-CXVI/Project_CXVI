// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Foundation/Combat/CXCombatMessageInterface.h"

// Add default functionality here for any ICXCombatMessageInterface functions that are not pure virtual.

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatMessageInterface)

void ICXCombatMessageInterface::CombatMessageResponse(const FCXCombatMessageData& MessageResponse)
{
	GetPhaseFinishedDelegate().Broadcast(MessageResponse);
}

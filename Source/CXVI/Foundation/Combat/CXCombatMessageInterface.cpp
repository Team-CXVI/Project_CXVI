// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Foundation/Combat/CXCombatMessageInterface.h"

// Add default functionality here for any ICXCombatMessageInterface functions that are not pure virtual.

void ICXCombatMessageInterface::CombatMessageResponse(const FCXCombatMessageData& MessageResponse)
{
	GetPhaseFinishedDelegate().Broadcast(MessageResponse);
}

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "CombatMessage/CXCombatMessageData.h"

#include "CXCombatMessageInterface.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnPhaseFinished, const FCXCombatMessageData&);

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCXCombatMessageInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CXVI_API ICXCombatMessageInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// Each participant owns its event and exposes it through this interface.
	virtual FOnTurnPhaseFinished& GetPhaseFinishedDelegate() = 0;

	// Shared C++ implementation; participants do not need to know the manager.
	// 턴매니저에게 받는 함수
	virtual void CombatMessageRequest(const FCXCombatMessageData& MessageRequest) = 0;

	// 턴매니저에게 보내는 함수
	virtual void CombatMessageResponse(const FCXCombatMessageData& MessageResponse);
};

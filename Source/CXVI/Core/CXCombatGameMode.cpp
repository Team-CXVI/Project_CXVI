// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Core/CXCombatGameMode.h"

#include "CXVI/Character/Combat/CXCombatCharacterPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatGameMode)

ACXCombatGameMode::ACXCombatGameMode()
{
	if (CombatCharacterPlayer)
	{
		DefaultPawnClass = CombatCharacterPlayer.GetClass();
	}

	//PlayerControllerClass = AGASPlayerController::StaticClass();
}

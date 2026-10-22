// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Core/CXCombatGameMode.h"

#include "CXVI/Character/Combat/CXCombatCharacterPlayer.h"

ACXCombatGameMode::ACXCombatGameMode()
{
	if (CombatCharacterPlayer)
	{
		DefaultPawnClass = CombatCharacterPlayer.GetClass();
	}

	//PlayerControllerClass = AGASPlayerController::StaticClass();
}

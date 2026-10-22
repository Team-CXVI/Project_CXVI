// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Core/CXCombatPlayerController.h"

#include "CXVI/Character/Combat/CXCommandCharacter.h"
#include "CXVI/Character/Combat/CXTempFieldCharacterPlayer.h"

ACXCombatPlayerController::ACXCombatPlayerController()
{
}

void ACXCombatPlayerController::EnterCombatMode()
{
	if (IsValid(CommandCharacter))
	{
		Possess(CommandCharacter);
	}
}

void ACXCombatPlayerController::EndCombatMode()
{
	if (IsValid(FieldCharacter))
	{
		Possess(FieldCharacter);
	}
}

void ACXCombatPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 입력설정
	FInputModeGameOnly GameOnlyInputMode;
	SetInputMode(GameOnlyInputMode);
}

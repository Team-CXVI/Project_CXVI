// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Core/CXCombatPlayerController.h"

#include "CXVI/Character/Combat/CXCommandCharacter.h"
#include "CXVI/Character/Combat/CXTempFieldCharacterPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatPlayerController)

ACXCombatPlayerController::ACXCombatPlayerController()
{
}


void ACXCombatPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 입력설정
	FInputModeGameOnly GameOnlyInputMode;
	SetInputMode(GameOnlyInputMode);
}

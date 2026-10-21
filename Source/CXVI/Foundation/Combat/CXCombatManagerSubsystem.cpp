// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Foundation/Combat/CXCombatManagerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatManagerSubsystem)

UCXCombatManagerSubsystem::UCXCombatManagerSubsystem()
{
}

void UCXCombatManagerSubsystem::RequestStartBattle(TArray<TSubclassOf<class ACXCombatCharacterBase>> Players, TArray<TSubclassOf<class ACXCombatCharacterBase>> Enemies, APlayerController* PlayerController)
{
}

void UCXCombatManagerSubsystem::RequestEndBattle()
{
}

void UCXCombatManagerSubsystem::PostInitialize()
{
	
}

void UCXCombatManagerSubsystem::HandlePlayerInputModeChanged(bool bAllowed)
{
}

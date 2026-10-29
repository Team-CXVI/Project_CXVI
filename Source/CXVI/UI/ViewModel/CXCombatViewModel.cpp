// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/CXCombatViewModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatViewModel)

void UCXCombatViewModel::SetCombatants(const TArray<TObjectPtr<ACXCombatCharacterBase>>& InPlayers, const TArray<TObjectPtr<ACXCombatCharacterBase>>& InEnemies)
{
}

void UCXCombatViewModel::Reset()
{
}

UCXCombatantViewModel* UCXCombatViewModel::CreateCombatantViewModel(AActor* InCombatant)
{
	return nullptr;
}

void UCXCombatViewModel::BroadcastAll()
{
}

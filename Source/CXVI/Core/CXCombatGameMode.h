// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CXCombatGameMode.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API ACXCombatGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ACXCombatGameMode();

private:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class ACXCombatCharacterPlayer> CombatCharacterPlayer;

};

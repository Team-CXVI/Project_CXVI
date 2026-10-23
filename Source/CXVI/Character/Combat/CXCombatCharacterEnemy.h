// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CXVI/Character/Combat/CXCombatCharacterBase.h"
#include "CXCombatCharacterEnemy.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API ACXCombatCharacterEnemy : public ACXCombatCharacterBase
{
	GENERATED_BODY()

public:
	ACXCombatCharacterEnemy();

protected:
	virtual void PostInitializeComponents() override;

	virtual void ActionPhase() override;

};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CXCombatPlayerController.generated.h"



/**
 * 
 */
UCLASS()
class CXVI_API ACXCombatPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ACXCombatPlayerController();


protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Character")
	TObjectPtr<class ACXTempFieldCharacterPlayer> FieldCharacter;

	UPROPERTY(EditDefaultsOnly, Category = "Character")
	TObjectPtr<class ACXCommandCharacter> CommandCharacter;

};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CXFieldCharacterBase.h"
#include "CXFieldEnemy.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API ACXFieldEnemy : public ACXFieldCharacterBase
{
	GENERATED_BODY()

public:
	ACXFieldEnemy();

protected:
	virtual void BeginPlay() override;

	// Field symbol identity only; encounter transition logic is not implemented here.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Encounter")
	FName EncounterID;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="PreemptiveAttack")
	TObjectPtr<class UChildActorComponent> PreemptiveAttackTrigger;


};

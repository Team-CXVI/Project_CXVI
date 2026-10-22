// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "CXPreemptiveAttackTriggerBox.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API ACXPreemptiveAttackTriggerBox : public ATriggerBox
{
	GENERATED_BODY()
	
public:
	ACXPreemptiveAttackTriggerBox(const FObjectInitializer& ObjectInitializer);

protected:

	UFUNCTION()
	void HandleActorEndOverlap(
		AActor* OverlappedActor,
		AActor* OtherActor);

	UFUNCTION()
	void HandleActorBeginOverlap(
		AActor* OverlappedActor,
		AActor* OtherActor
	);

protected:
	UPROPERTY()
	TWeakObjectPtr<class ACXFieldEnemy> EnemyCharacter;

	UPROPERTY()
	TWeakObjectPtr<class ACXFieldPlayerCharacter> PlayerCharacter;

};

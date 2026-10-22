// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "CXSaveFlagTriggerBox.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API ACXSaveFlagTriggerBox : public ATriggerBox
{
	GENERATED_BODY()
	
public:
	ACXSaveFlagTriggerBox(const FObjectInitializer& ObjectInitializer);

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
};

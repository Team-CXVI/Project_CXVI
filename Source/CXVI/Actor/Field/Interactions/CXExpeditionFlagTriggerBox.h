// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "CXExpeditionFlagTriggerBox.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API ACXExpeditionFlagTriggerBox : public ATriggerBox
{
	GENERATED_BODY()
	
public:
	ACXExpeditionFlagTriggerBox(const FObjectInitializer& ObjectInitializer);

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

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CXPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API ACXPlayerController : public APlayerController
{
	GENERATED_BODY()
	
	
protected:
	virtual void ReceivedPlayer() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
};

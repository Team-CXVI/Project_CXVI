// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CXFieldGrapplingUIInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCXFieldGrapplingUIInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CXVI_API ICXFieldGrapplingUIInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction|UI")
	void UpdateInteractionUI(bool bCanInteract, AActor* TargetActor);
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CXCombatCharacterBase.h"

#include "GameplayAbilitySpecHandle.h"
#include "InputActionValue.h"

#include "CXCombatCharacterPlayer.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API ACXCombatCharacterPlayer : public ACXCombatCharacterBase
{
	GENERATED_BODY()
	
public:
	ACXCombatCharacterPlayer();

	// Getter
	FORCEINLINE const bool GetIsAimMode() const { return bIsAimMode; }

	void Look(const FInputActionValue& Value);

	bool PerformAimTrace(FHitResult& OutHit);

	void FireAbility();

protected:
	virtual void PostInitializeComponents() override;


protected:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<class UCameraComponent> Camera;

	// Ability
private:

	bool bIsAimMode = false;
};

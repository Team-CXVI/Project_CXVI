// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CXVI/Character/Combat/CXCombatCharacterBase.h"

#include "InputActionValue.h"

#include "CXTempFieldCharacterPlayer.generated.h"

class UInputAction;

/**
 * 
 */
UCLASS()
class CXVI_API ACXTempFieldCharacterPlayer : public ACXCombatCharacterBase
{
	GENERATED_BODY()
	
public:
	ACXTempFieldCharacterPlayer();

	const TArray<TSubclassOf<ACXCombatCharacterBase>> GetCharacterData() const;

protected:
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override;

	virtual void UnPossessed() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	void Move(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<class UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<class UCXCharacterPartyData> PartyData;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<class UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BluePrintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BluePrintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

};

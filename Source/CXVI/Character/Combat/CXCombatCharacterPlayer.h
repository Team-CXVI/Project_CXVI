// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CXCombatCharacterBase.h"

#include "GameplayAbilitySpecHandle.h"
#include "InputActionValue.h"

#include "CXCombatCharacterPlayer.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTargetSelectionRequested, ACXCombatCharacterPlayer*, const TArray<TWeakObjectPtr<APawn>>&);

/**
 * 
 */
UCLASS()
class CXVI_API ACXCombatCharacterPlayer : public ACXCombatCharacterBase
{
	GENERATED_BODY()
	
public:
	ACXCombatCharacterPlayer();

	FOnTargetSelectionRequested OnTargetSelectionRequested;

	// Getter
	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool GetIsAimMode() const { return bIsAimMode; }

	void Look(const FInputActionValue& Value);

	bool PerformAimTrace(FHitResult& OutHit);

	bool ToggleAimMode();
	void FireAbility();

	bool CanActivateSelectSkill(int32 SlotIndex) const;

	void SkipTurn();

protected:
	virtual void PostInitializeComponents() override;

	bool ActivateSelectedSkill(int32 SlotIndex);

	virtual bool ExecuteCombatAbility() override;

	virtual void SelectActionPhase() override;
	virtual void SelectTargetPhase() override;
	//virtual void ActionPhase() override;


protected:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<class UCameraComponent> Camera;

	// Ability
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Skills")
	TArray<TSubclassOf<class UGameplayAbility>> SkillAbilities;
private:

	bool bIsAimMode = false;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CXCombatCharacterBase.h"
#include "Foundation/Combat/CXCombatTypes.h"

#include "GameplayAbilitySpec.h"
#include "InputActionValue.h"

#include "CXCombatCharacterPlayer.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTargetSelectionRequested, ACXCombatCharacterPlayer*, const TArray<TWeakObjectPtr<APawn>>&);

struct FCXSkillRow;

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
	void TryRunaway();

	void SetInitialAttributes(const FCXInitialAttributes& InAttributes) { InitialAttributes = InAttributes; }
	void SetSkillRowPrefix(FName InPrefix) { SkillRowPrefix = InPrefix; }

	const FCXSkillRow* GetSkillRowForSlot(int32 SlotIndex) const;
	bool PrepareSelectedSkill(int32 SlotIndex);

	const float* FindPreparedSkillAPCost(FGameplayAbilitySpecHandle Handle) const { return PreparedSkillAPCosts.Find(Handle); }

	FVector GetCameraFocusLocation(ECXCameraFocusPoint FocusPoint) const;


protected:
	virtual void PostInitializeComponents() override;

	bool ActivateSelectedSkill(int32 SlotIndex);

	virtual bool ExecuteCombatAbility() override;

	virtual void SelectActionPhase() override;
	virtual void SelectTargetPhase() override;
	//virtual void ActionPhase() override;

private:
	const FGameplayAbilitySpec* FindPreparedSkillSpec(int32 SlotIndex) const;


protected:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<class UCameraComponent> Camera;

	// Ability
protected:
	//UPROPERTY(EditDefaultsOnly, Category = "Combat|Skills")
	//TArray<TSubclassOf<class UGameplayAbility>> SkillAbilities;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<class USceneComponent> CameraFocusBody;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<class USceneComponent> CameraFocusChest;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<class USceneComponent> CameraFocusHand;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute")
	TObjectPtr<class UCXCombatPlayerAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<class UCXJustTimingComponent> JustTimingComponent;
private:

	bool bIsAimMode = false;

	FCXInitialAttributes InitialAttributes;

	FName SkillRowPrefix = NAME_None;

	TMap<int32, FGameplayAbilitySpecHandle> PreparedSkillHandles;
	TMap<FGameplayAbilitySpecHandle, float> PreparedSkillAPCosts;
};

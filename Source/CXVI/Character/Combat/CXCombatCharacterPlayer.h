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

	// 전투 아이템 메뉴 슬롯의 ItemId (DT_Item Row 이름). 비어 있는 슬롯이면 NAME_None
	FName GetItemIdForSlot(int32 SlotIndex) const;
	bool PrepareSelectedSkill(int32 SlotIndex);

	const float* FindPreparedSkillAPCost(FGameplayAbilitySpecHandle Handle) const { return PreparedSkillAPCosts.Find(Handle); }

	bool UseItem(FName ItemId, ACXCombatCharacterBase* Target);

protected:
	virtual void PostInitializeComponents() override;

	bool ActivateSelectedSkill(int32 SlotIndex);

	virtual bool ExecuteCombatAbility() override;

	virtual void BeginTurnPhase() override;
	virtual void SelectActionPhase() override;
	virtual void SelectTargetPhase() override;
	virtual bool ActionPhase() override;

	virtual void StartHitReactMontage() override;

	// 원거리 공격 판단
	//virtual bool ShouldApproachTarget() const override;

	virtual const FGameplayAbilitySpec* GetSelectedAttackSpec() const override;

	// 스킬이면 슬롯의 스킬 행 DisplayName (커맨드 메뉴와 같은 GetSkillRowForSlot 기준)
	virtual FText GetSelectedActionName() const override;



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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> HitReactMontage;




	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute")
	TObjectPtr<class UCXCombatPlayerAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<class UCXJustTimingComponent> JustTimingComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<class USceneComponent> ActionFocusPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<class USceneComponent> SkillFocusPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<class USceneComponent> ItemFocusPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<class USceneComponent> AttackingFocus;


	UPROPERTY(EditDefaultsOnly, Category = "Combat|Item")
	TMap<FName, TSubclassOf<class UGameplayEffect>> ItemEffects;

private:
	bool bUsingItem = false;

	bool bIsAimMode = false;

	FCXInitialAttributes InitialAttributes;

	FName SkillRowPrefix = NAME_None;

	TMap<int32, FGameplayAbilitySpecHandle> PreparedSkillHandles;
	TMap<FGameplayAbilitySpecHandle, float> PreparedSkillAPCosts;

};

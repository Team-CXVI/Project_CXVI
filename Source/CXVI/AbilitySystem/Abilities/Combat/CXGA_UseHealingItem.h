// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CXGA_UseHealingItem.generated.h"

class ACXCombatCharacterPlayer;
class ACXCombatCharacterBase;

/**
 * 
 */
UCLASS()
class CXVI_API UCXGA_UseHealingItem : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UCXGA_UseHealingItem();

	static bool CanUseHealingItem(
		const ACXCombatCharacterPlayer* User,
		FName ItemId,
		const ACXCombatCharacterBase* Target = nullptr);


protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr
	) const override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

protected:
	// 에디터에서 만든 공통 회복 GE지정
	UPROPERTY(EditDefaultsOnly, Category = "Item|Recovery")
	TSubclassOf<class UGameplayEffect> HealingEffectClass;

};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CXPlayerPreemptiveAttackAbility.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXPlayerPreemptiveAttackAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UCXPlayerPreemptiveAttackAbility();

	void SlowMotion();

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;


protected:
	// 애니메이션
	UFUNCTION()
	void OnAssassinationInterrupted()
	{
		if (IsActive())
		{
			EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, true);
		}
	}

	UFUNCTION()
	void OnAssassinationCompleted()
	{
		if (IsActive())
		{
			EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, false);
		}
	}
	
	UFUNCTION()
	void OnSlowMotionEvent(FGameplayEventData Payload);

	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PreemptiveAttack")
	TObjectPtr<UAnimMontage> AssassinaionMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PreemptiveAttack",meta=(ClampMin="0.01",ClampMax="1.0"))
	float SlowMotionRate = 0.2f;

	float OriginalTimeDilation = 1.0f;
	bool bAppliedSlowMotion = false;


};

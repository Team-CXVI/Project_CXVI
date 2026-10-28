// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CXGA_Dodge.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXGA_Dodge : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UCXGA_Dodge();

private:
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


private:
	// 공격 몽타주가 정상적으로 끝났을 경우
	UFUNCTION()
	void OnAbilityCompleted();

	// 공격 몽타주가 재생이 중단되거나 Task가 취소 됐을 경우
	UFUNCTION()
	void OnAbilityInterrupted();

private:
	void RegisterMontageTask();



private:
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<class UAnimMontage> Montage;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<class UGameplayEffect> DodgeWindowEffect;

	FActiveGameplayEffectHandle DodgeWindowHandle;
};

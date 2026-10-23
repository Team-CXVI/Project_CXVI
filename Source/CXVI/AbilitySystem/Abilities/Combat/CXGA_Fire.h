// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"

#include "CXGA_Fire.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXGA_Fire : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UCXGA_Fire();

protected:
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	// 공격 몽타주가 정상적으로 끝났을 경우
	UFUNCTION()
	void OnAttackCompleted();
	// 공격 몽타주가 재생이 중단되거나 Task가 취소 됐을 경우
	UFUNCTION()
	void OnAttackInterrupted();

	// 몽타주 태스크 등록 함수
	void RegisterMontageTask();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Montage")
	TObjectPtr<class UAnimMontage> AbilityMontage;

	UPROPERTY(BlueprintReadOnly, Category = "Montage", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UAbilityTask_PlayMontageAndWait> MontageTask;

};

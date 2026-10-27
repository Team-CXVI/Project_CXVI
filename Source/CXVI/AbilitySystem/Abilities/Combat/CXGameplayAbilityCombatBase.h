// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"

#include "CXGameplayAbilityCombatBase.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXGameplayAbilityCombatBase : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UCXGameplayAbilityCombatBase();

	float GetManaCost() const;

	virtual bool CheckCost(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

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

	virtual void InputPressed(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo
	) override;

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
	void OnAttackCompleted();

	// 공격 몽타주가 재생이 중단되거나 Task가 취소 됐을 경우
	UFUNCTION()
	void OnAttackInterrupted();

private:
	void RegisterMontageTask();

	UFUNCTION()
	void OnAttackEvent(FGameplayEventData Payload);

	bool RegisterHitEventTask();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Montage")
	TObjectPtr<class UCXAbilityData> AbilityData;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TSubclassOf<class UGameplayEffect> SkillDamageEffect;

private:
	// 이번 스킬 사용에서 이미 처리한 타격 번호.
	TSet<int32> ProcessedHitIndices;
};

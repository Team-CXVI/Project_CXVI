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

	float GetAPCost() const;

	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo
	) const override;

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

	void UpdateNextAttackSection(int32 HitIndex, bool bSuccess);

	bool RegisterComboEventTask();

	UFUNCTION()
	void OnComboBranchEvent(FGameplayEventData Payload);

	float GetAPCostForSpec(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Montage")
	TObjectPtr<class UCXAbilityData> AbilityData;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TSubclassOf<class UGameplayEffect> SkillDamageEffect;

private:
	// 이번 스킬 사용에서 이미 처리한 타격 번호.
	TSet<int32> ProcessedHitIndices;

	// 동일한 동작의 연께 판정을 중복 처리 않음
	TSet<int32> ProcessedComboStages;

	// 이전 동작의 판정 성공으로 열린 공격섹션.
	// 해당 섹션의 여러타격이 같은 결과를 사용
	TSet<FName> SuccessfulComboSections;
};

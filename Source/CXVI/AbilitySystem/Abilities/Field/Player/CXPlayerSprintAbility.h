// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ActiveGameplayEffectHandle.h"
#include "CXPlayerSprintAbility.generated.h"


class UCharacterMovementComponent;
class UAbilityTask_WaitAttributeChange;
/**
 * 
 */
UCLASS()
class CXVI_API UCXPlayerSprintAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UCXPlayerSprintAbility();




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

	UFUNCTION(BlueprintImplementableEvent, Category = Sprint)
	void OnSprintStarted();

protected:
	// 스프린트 시 플레이어의 이동속도.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Sprint)
	float SprintSpeed = 600.0f;

private:
	// 캐릭터의 이동속도만 알고 싶기 때문에 TWeakObjectPtr.
	UPROPERTY()
	TWeakObjectPtr<UCharacterMovementComponent> SprintMovement;
	
	// 기존의 이동속도를 저장하기 위한 변수.
	float OriginalWalkSpeed = 0.0f;



};

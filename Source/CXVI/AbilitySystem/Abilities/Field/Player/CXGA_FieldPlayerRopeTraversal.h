// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CXVI/Actor/Field/Interactions/CXRopeTraversalActor.h"
#include "Abilities/GameplayAbility.h"
#include "CXGA_FieldPlayerRopeTraversal.generated.h"

/**
 * 
 */
class ACXFieldPlayerCharacter;
class UAbilityTask_ApplyRootMotionMoveToForce;

UCLASS()
class CXVI_API UCXGA_FieldPlayerRopeTraversal : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UCXGA_FieldPlayerRopeTraversal();

protected:
    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	// 플레이어 로프 이동.
protected:
	void StartMoveToEntry();

	UFUNCTION()
	void OnEntryMoveFinished();

	UFUNCTION()
	void OnTraversalInterrupted();

	UPROPERTY(EditDefaultsOnly, Category = "Rope")
	float EntryMoveDuration = 0.3f;

	UPROPERTY(Transient)
	FVector EntryMoveGoal = FVector::ZeroVector;

	void StartMoveToDestination();

	UFUNCTION()
	void OnDestinationFinished();

	UPROPERTY(EditDefaultsOnly, Category = "Rope")
	float TraversalSpeed = 600.0f;

	UPROPERTY(Transient)
	FVector DestinationMoveGoal = FVector::ZeroVector;

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<ACXFieldPlayerCharacter> TraversalCharacter;

	UPROPERTY(Transient)
	TWeakObjectPtr<ACXRopeTraversalActor> TraversalRope;

	UPROPERTY(Transient)
	ECXRopeTraversalDirection TraversalDirection = ECXRopeTraversalDirection::None;

	UPROPERTY(Transient)
	FVector TraversalEntry = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector TraversalDestination = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector TraversalExit = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_ApplyRootMotionMoveToForce> TraversalMoveTask;

	bool bControlsTraversalMovement = false;
};

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

	// 진입점.
	void StartMoveToEntry();

	UFUNCTION()
	void OnEntryMoveFinished();

	UFUNCTION()
	void OnTraversalInterrupted();

	UPROPERTY(EditDefaultsOnly, Category = "Rope")
	float EntryMoveDuration = 0.3f;

	UPROPERTY(Transient)
	FVector EntryMoveGoal = FVector::ZeroVector;

	// 이동 경로.
	void StartMoveToDestination();

	UFUNCTION()
	void OnDestinationFinished();

	UPROPERTY(EditDefaultsOnly, Category = "Rope")
	float TraversalSpeed = 600.0f;

	UPROPERTY(Transient)
	FVector DestinationMoveGoal = FVector::ZeroVector;

	// 탈출점.
	void StartMoveToExit();

	UFUNCTION()
	void OnExitMoveFinished();

	UPROPERTY(EditDefaultsOnly, Category = "Rope")
	float ExitMoveDuration = 0.3f;

	UPROPERTY(Transient)
	FVector ExitMoveGoal = FVector::ZeroVector;

	// 착지.
	UFUNCTION()
	void HandleTraversalLanded(const FHitResult& Hit);

	bool bWaitingForLanding = false;

	UPROPERTY(EditDefaultsOnly, Category = "Rope")
	float LandingTimeOut = 3.0f;

	// 발판 위 접근점에서 로프에 붙는 이동.
	void StartMoveToRopeAttachment();

	UFUNCTION()
	void OnRopeAttachmentFinished();

	UPROPERTY(Transient)
	FVector AttachmentMoveGoal = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Rope")
	float AttachmentMoveDuration = 0.3f;


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
	FVector TraversalApproach = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector TraversalDestination = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector TraversalExit = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_ApplyRootMotionMoveToForce> TraversalMoveTask;

	bool bControlsTraversalMovement = false;

	UPROPERTY(EditDefaultsOnly, Category = "Rope")
	TObjectPtr<class UCurveVector> ExitUpCurvePath;

	UPROPERTY(EditDefaultsOnly, Category = "Rope")
	TObjectPtr<class UCurveVector> ExitDownCurvePath;

};

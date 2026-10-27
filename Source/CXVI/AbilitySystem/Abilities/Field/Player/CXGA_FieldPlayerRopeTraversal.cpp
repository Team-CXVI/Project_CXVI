// Fill out your copyright notice in the Description page of Project Settings.


#include "CXGA_FieldPlayerRopeTraversal.h"
#include "CXVI/Character/Field/CXFieldPlayerCharacter.h"
#include "CXVI/Actor/Field/Interactions/CXRopeTraversalActor.h"
#include "CXVI/Tags/CXFieldTags.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/RootMotionSource.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGA_FieldPlayerRopeTraversal)

UCXGA_FieldPlayerRopeTraversal::UCXGA_FieldPlayerRopeTraversal()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	ActivationRequiredTags.AddTag(CXVIFieldTag::State_CanUseRope);

	ActivationOwnedTags.AddTag(CXVIFieldTag::State_MovingBlock);

	ActivationBlockedTags.AddTag(CXVIFieldTag::State_MovingBlock);
}

bool UCXGA_FieldPlayerRopeTraversal::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const ACXFieldPlayerCharacter* Player = ActorInfo ? Cast<ACXFieldPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;

	return IsValid(Player)&&IsValid(Player->GetRopeTraversalTarget()) && Player->GetRopeTraversalDirection() != ECXRopeTraversalDirection::None ;
}

void UCXGA_FieldPlayerRopeTraversal::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ACXFieldPlayerCharacter* Player = ActorInfo ? Cast<ACXFieldPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!IsValid(Player))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACXRopeTraversalActor* Rope = IsValid(Player) ? Player->GetRopeTraversalTarget() : nullptr;
	if (!IsValid(Rope))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const ECXRopeTraversalDirection Direction = Player->GetRopeTraversalDirection();

	FVector Entry;
	FVector Destination;
	FVector Exit;

	if (!Rope->GetTraversalPoints(Direction, Entry, Destination, Exit))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TraversalCharacter = Player;
	TraversalRope = Rope;
	TraversalDirection = Direction;

	TraversalEntry = Entry;
	TraversalDestination = Destination;
	TraversalExit = Exit;


	UE_LOG(LogTemp, Log,
		TEXT("[RopeAbility] %s / Entry=%s / Destination=%s / Exit=%s"),
		Direction == ECXRopeTraversalDirection::Up
		? TEXT("상승") : TEXT("하강"),
		*TraversalEntry.ToString(),
		*TraversalDestination.ToString(),
		*TraversalExit.ToString());

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UCXGA_FieldPlayerRopeTraversal::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo))
	{
		return;
	}

	// GAS 내부 잠금 중이면 정리 전체를 나중으로 미룸.
	if (ScopeLockCount > 0)
	{
		WaitingToExecute.Add(
			FPostLockDelegate::CreateUObject(
				this,
				&ThisClass::EndAbility,
				Handle,
				ActorInfo,
				ActivationInfo,
				bReplicateEndAbility,
				bWasCancelled));
		return;
	}

	// 실행 중인 이동 제어를 종료 합니다.
	if (IsValid(TraversalMoveTask))
	{
		TraversalMoveTask->EndTask();
		TraversalMoveTask = nullptr;
	}

	const bool bShouldRestoreMovement = bControlsTraversalMovement;
	bControlsTraversalMovement = false;

	if (ACXFieldPlayerCharacter* Player = TraversalCharacter.Get())
	{
		UCharacterMovementComponent* Movement = Player->GetCharacterMovement();

		// 이번 어빌리티가 변경했고, 현재도 비행 중일 때만 복구.
		if (bShouldRestoreMovement && Movement && Movement->MovementMode == MOVE_Flying)
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_Falling);
		}
	}
	
	// 이번 실행에서 보관함 정보를 비웁니다.
	TraversalCharacter.Reset();
	TraversalRope.Reset();
	TraversalDirection = ECXRopeTraversalDirection::None;
	TraversalEntry = FVector::ZeroVector;
	TraversalDestination = FVector::ZeroVector;
	TraversalExit = FVector::ZeroVector;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

}

void UCXGA_FieldPlayerRopeTraversal::StartMoveToEntry()
{
	if (!IsActive())
	{
		return;
	}

}

void UCXGA_FieldPlayerRopeTraversal::OnEntryMoveFinished()
{
}

void UCXGA_FieldPlayerRopeTraversal::OnTraversalInterrupted()
{
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "CXGA_FieldPlayerRopeTraversal.h"
#include "CXVI/Character/Field/CXFieldPlayerCharacter.h"
#include "CXVI/Actor/Field/Interactions/CXRopeTraversalActor.h"
#include "CXVI/Tags/CXFieldTags.h"

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

	UE_LOG(LogTemp, Log,
		TEXT("[RopeAbility] %s / Entry=%s / Destination=%s / Exit=%s"),
		Direction == ECXRopeTraversalDirection::Up
		? TEXT("상승") : TEXT("하강"),
		*Entry.ToString(),
		*Destination.ToString(),
		*Exit.ToString());

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

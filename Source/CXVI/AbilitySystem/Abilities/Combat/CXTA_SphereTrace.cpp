// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/AbilitySystem/Abilities/Combat/CXTA_SphereTrace.h"

#include "Abilities/GameplayAbility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXTA_SphereTrace)

ACXTA_SphereTrace::ACXTA_SphereTrace()
{
	ShouldProduceTargetDataOnServer = true;
}

void ACXTA_SphereTrace::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);

	SourceActor = Ability->GetCurrentActorInfo()->AvatarActor.Get();
}

void ACXTA_SphereTrace::ConfirmTargetingAndContinue()
{
	check(ShouldProduceTargetData());
	if (IsConfirmTargetingAllowed())
	{
		FHitResult HitResult = GetSphereTraceResult(SourceActor);
		FGameplayAbilityTargetDataHandle DataHandle(new FGameplayAbilityTargetData_SingleTargetHit(HitResult));
		TargetDataReadyDelegate.Broadcast(DataHandle);
	}
}

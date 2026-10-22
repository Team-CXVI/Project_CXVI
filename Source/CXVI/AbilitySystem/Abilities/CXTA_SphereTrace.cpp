// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/AbilitySystem/Abilities/CXTA_SphereTrace.h"

#include "Abilities/GameplayAbility.h"

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

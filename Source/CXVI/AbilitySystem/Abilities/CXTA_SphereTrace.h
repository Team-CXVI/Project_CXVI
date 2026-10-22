// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "CXTA_SphereTrace.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API ACXTA_SphereTrace : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()

public:
	ACXTA_SphereTrace();

protected:
	virtual void StartTargeting(UGameplayAbility* Ability) override;

	virtual void ConfirmTargetingAndContinue() override;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	FHitResult GetSphereTraceResult(AActor* InSourceActor);
};

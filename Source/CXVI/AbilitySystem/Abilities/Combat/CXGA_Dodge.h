// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CXGA_Dodge.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXGA_Dodge : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UCXGA_Dodge();


private:
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr
	) const override;

};

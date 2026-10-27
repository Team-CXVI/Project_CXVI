// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Effects/Combat/CXMMC_ManaCost.h"

#include "AbilitySystem/Abilities/Combat/CXGameplayAbilityCombatBase.h"

#include "GameplayEffect.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXMMC_ManaCost)

float UCXMMC_ManaCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const UCXGameplayAbilityCombatBase* Ability = Cast<UCXGameplayAbilityCombatBase>(Spec.GetContext().GetAbility());

	if (!Ability)
	{
		UE_LOG(LogTemp, Warning, TEXT("마나비용 계산: 전투 스킬 Ga를 찾지 못함."));
		return 0.f;
	}

	// 데이터는 양수로 저장하고 GE에는 감소량으로 전달.
	return -Ability->GetManaCost();
}

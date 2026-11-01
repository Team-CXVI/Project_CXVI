// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/Combat/CXCombatEnemyAttributeSet.h"

#include "GameplayEffectExtension.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatEnemyAttributeSet)

UCXCombatEnemyAttributeSet::UCXCombatEnemyAttributeSet()
{
}

void UCXCombatEnemyAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetDodgeChanceAttribute())
	{
		NewValue = FMath::Max<float>(0.f, NewValue);
	}
}

void UCXCombatEnemyAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

}

void UCXCombatEnemyAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
}

void UCXCombatEnemyAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
}

bool UCXCombatEnemyAttributeSet::TryAvoidDamage(const FGameplayEffectModCallbackData& Data, float IncomingDamage)
{
	bool Result = Super::TryAvoidDamage(Data, IncomingDamage);

	const float TempDodgeChance = FMath::Clamp(GetDodgeChance(), 0.f, 100.f);

	const bool bIsDodge = TempDodgeChance >= 100.f || (TempDodgeChance > 0.f && FMath::FRand() * 100.f < TempDodgeChance);


	if (bIsDodge)
	{
		Result = true;
	}

	return Result;
}

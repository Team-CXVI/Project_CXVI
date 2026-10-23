// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/AbilitySystem/Attributes/Combat/CXCombatAttributeSet.h"

#include "GameplayEffectExtension.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatAttributeSet)

UCXCombatAttributeSet::UCXCombatAttributeSet()
	: MaxHealth(100.f), CombatSpeed(100.f), AttackRange(100.f), AttackRadius(50.f), AttackRate(40.f)
{
	InitHealth(GetMaxHealth());
}

void UCXCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max<float>(1.f, NewValue);
	}
	else if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
}

void UCXCombatAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	if (Attribute == GetMaxHealthAttribute())
	{
		OnMaxHealthChanged.Broadcast(OldValue, NewValue);
	}
	else if (Attribute == GetHealthAttribute())
	{
		OnHealthChanged.Broadcast(OldValue, NewValue);
	}
}

void UCXCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		float AppliedDamage = 0.f;
		float RemainingDamage = GetDamage();

		float OldHealth = GetHealth();
		float NewHealth = OldHealth - RemainingDamage;
		SetHealth(NewHealth);

		AppliedDamage = OldHealth - NewHealth;
		OnDamageApplied.Broadcast(Data.EffectSpec.GetEffectContext().GetInstigator(), AppliedDamage);
		SetDamage(0.f);
	}

	if (GetHealth() <= 0.f)
	{
		OnOutOfHealth.Broadcast(Data.EffectSpec.GetEffectContext().GetInstigator());
	}
}

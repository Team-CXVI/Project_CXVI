// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/AbilitySystem/Attributes/Combat/CXCombatAttributeSet.h"

#include "GameplayEffectExtension.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatAttributeSet)

UCXCombatAttributeSet::UCXCombatAttributeSet()
	: MaxHealth(100.f), Mana(0.f), MaxMana(100.f),
	CombatSpeed(100.f), AttackRange(100.f), AttackRadius(50.f), AttackRate(40.f)
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
	else if (Attribute == GetMaxManaAttribute())
	{
		NewValue = FMath::Max<float>(0.f, NewValue);
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
	}
}

void UCXCombatAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxHealthAttribute())
	{
		OnMaxHealthChanged.Broadcast(OldValue, NewValue);
	}
	else if (Attribute == GetHealthAttribute())
	{
		OnHealthChanged.Broadcast(OldValue, NewValue);
	}
	else if (Attribute == GetMaxManaAttribute())
	{
		if (GetMana() > NewValue)
		{
			SetMana(NewValue);
		}
	}
	else if (Attribute == GetManaAttribute())
	{
		if (!FMath::IsNearlyEqual(OldValue, NewValue))
		{
			UE_LOG(LogTemp, Display,
				TEXT("%s 마나: %.0f -> %.0f / %.0f"),
				*GetNameSafe(GetOwningActor()),
				OldValue, NewValue, GetMaxMana());
		}
	}
}

void UCXCombatAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	if (Attribute == GetMaxManaAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
	}
}

void UCXCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
		return;
	}

	if (Data.EvaluatedData.Attribute == GetMaxManaAttribute())
	{
		SetMaxMana(FMath::Max(0.f, GetMaxMana()));
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
		return;
	}

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		const float OldHealth = GetHealth();
		const float IncomingDamage = FMath::Max(0.f, GetDamage());

		// 데미지는 이번처리에서 소비하고 초기화
		SetDamage(0.f);

		const float NewHealth = FMath::Clamp(OldHealth - IncomingDamage, 0.f, GetMaxHealth());

		SetHealth(NewHealth);

		const float AppliedDamage = FMath::Max(0.f, OldHealth - GetHealth());

		AActor* Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();

		// 실제 체력을 감소시켰을 때만 명중 결과 알림.
		if (AppliedDamage > 0.f)
		{
			OnDamageApplied.Broadcast(Instigator, AppliedDamage);
		}

		if (OldHealth > 0.f && GetHealth() <= 0.f)
		{
			OnOutOfHealth.Broadcast(Instigator);
		}
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/Combat/CXCombatAttributeSetBase.h"

#include "Tags/CXGameplayTags.h"

#include "GameplayEffectExtension.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatAttributeSetBase)

UCXCombatAttributeSetBase::UCXCombatAttributeSetBase()
	: Health(100.f), MaxHealth(100.f), CombatSpeed(1.f)
{
}

void UCXCombatAttributeSetBase::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max<float>(1.f, NewValue);
	}
	else if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetCombatSpeedAttribute())
	{
		NewValue = FMath::Max<float>(1.f, NewValue);
	}
	else if (Attribute == GetCriticalChanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 100.f);
	}
}

void UCXCombatAttributeSetBase::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
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
}

void UCXCombatAttributeSetBase::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

}

void UCXCombatAttributeSetBase::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		const float IncomingDamage = FMath::Max(0.f, GetDamage());

		// 데미지는 이번처리에서 소비하고 초기화
		SetDamage(0.f);

		if (IncomingDamage <= 0.f || GetHealth() <= 0.f)
		{
			return;
		}


		if (TryAvoidDamage(Data, IncomingDamage))
		{
			return;
		}

		const float FinalDamage = CalculateFinalDamage(Data, IncomingDamage);

		if (FinalDamage <= 0.f)
		{
			return;
		}

		const float OldHealth = GetHealth();
		SetHealth(FMath::Clamp(OldHealth - FinalDamage, 0.f, GetMaxHealth()));

		const float AppliedDamage = FMath::Max(0.f, OldHealth - GetHealth());

		AActor* Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();

		UE_LOG(LogTemp, Display, TEXT("%s 피격 : 계산 피해 %.1f / 실제 체력 감소 %.1f"), *GetNameSafe(GetOwningActor()), FinalDamage, AppliedDamage);

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

bool UCXCombatAttributeSetBase::TryAvoidDamage(const FGameplayEffectModCallbackData& Data, float IncomingDamage)
{
	return false;
}

float UCXCombatAttributeSetBase::CalculateFinalDamage(const FGameplayEffectModCallbackData& Data, float IncomingDamage) const
{
	float FinalDamage = FMath::Max(0.f, IncomingDamage);
	
	// 공격자의 치명타 확률
	const UAbilitySystemComponent* SourceASC = Data.EffectSpec.GetEffectContext().GetOriginalInstigatorAbilitySystemComponent();

	const UCXCombatAttributeSetBase* SourceAttributes = SourceASC ? SourceASC->GetSet<UCXCombatAttributeSetBase>() : nullptr;

	if (SourceAttributes)
	{
		const float TempCriticalChance = FMath::Clamp(SourceAttributes->GetCriticalChance(), 0.f, 100.f);

		const bool bIsCritical = TempCriticalChance >= 100.f || (TempCriticalChance > 0.f && FMath::FRand() * 100.f < TempCriticalChance);

		if (bIsCritical)
		{
			const float CriticalMutiplier = FMath::Max(1.f, SourceAttributes->GetCriticalDamageMultiplier());

			FinalDamage *= CriticalMutiplier;
		}
	}

	// 방어력을 반영
	// 테스트 방어령 0 -> 원래 피해 , 100 -> 피해 절반
	const float TargetDefense = FMath::Max(0.f, GetDefense());

	FinalDamage *= 100.f / (100.f + TargetDefense);

	return FMath::Max(0.f, FinalDamage);
}

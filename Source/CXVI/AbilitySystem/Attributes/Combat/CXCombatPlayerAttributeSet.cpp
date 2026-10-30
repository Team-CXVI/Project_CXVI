// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/Combat/CXCombatPlayerAttributeSet.h"

#include "Tags/CXGameplayTags.h"

#include "GameplayEffectExtension.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatPlayerAttributeSet)

UCXCombatPlayerAttributeSet::UCXCombatPlayerAttributeSet()
	: AP(0.f), MaxAP(10.f)
{
}

void UCXCombatPlayerAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetAPAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxAP());
	}
	else if (Attribute == GetMaxAPAttribute())
	{
		NewValue = FMath::Max<float>(0.f, NewValue);
	}
}

void UCXCombatPlayerAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxAPAttribute())
	{
		if (GetAP() > NewValue)
		{
			SetAP(NewValue);
		}
	}
	else if (Attribute == GetAPAttribute())
	{
		if (!FMath::IsNearlyEqual(OldValue, NewValue))
		{
			UE_LOG(LogTemp, Display,
				TEXT("%s 마나: %.0f -> %.0f / %.0f"),
				*GetNameSafe(GetOwningActor()),
				OldValue, NewValue, GetMaxAP());
		}
	}
}

void UCXCombatPlayerAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	if (Attribute == GetMaxAPAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
	else if (Attribute == GetAPAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxAP());
	}
}

void UCXCombatPlayerAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetAPAttribute())
	{
		SetAP(FMath::Clamp(GetAP(), 0.f, GetMaxAP()));
		return;
	}

	if (Data.EvaluatedData.Attribute == GetMaxAPAttribute())
	{
		SetMaxAP(FMath::Max(0.f, GetMaxAP()));
		SetAP(FMath::Clamp(GetAP(), 0.f, GetMaxAP()));
		return;
	}

}

bool UCXCombatPlayerAttributeSet::TryAvoidDamage(const FGameplayEffectModCallbackData& Data, float IncomingDamage)
{
	bool Result = Super::TryAvoidDamage(Data, IncomingDamage);

	UAbilitySystemComponent* TargetASC = GetOwningAbilitySystemComponent();

	if (TargetASC && TargetASC->HasMatchingGameplayTag(CXVIGameplayTags::State_Combat_Dodging))
	{
		UE_LOG(LogTemp, Display, TEXT("%s 회피 성공: 데미지 %.1f 무시"), *GetNameSafe(GetOwningActor()), IncomingDamage);

		Result = true;
	}

	if (IncomingDamage > 0.f && TargetASC && TargetASC->HasMatchingGameplayTag(CXVIGameplayTags::State_Combat_Parrying))
	{
		SetAP(FMath::Clamp(GetAP() + 1.f, 0.f, GetMaxAP()));

		AActor* Insigator = Data.EffectSpec.GetEffectContext().GetInstigator();

		OnParrySucceeded.Broadcast(Insigator);

		Result = true;
	}

	return Result;
}

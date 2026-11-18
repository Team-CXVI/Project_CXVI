// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/Combat/CXCombatPlayerAttributeSet.h"

#include "Tags/CXGameplayTags.h"

#include "GameplayEffectExtension.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatPlayerAttributeSet)

UCXCombatPlayerAttributeSet::UCXCombatPlayerAttributeSet()
	: AP(3.f), MaxAP(10.f)
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

	if (!TargetASC)
	{
		return Result;
	}

	FGameplayTagContainer DamageTags;
	Data.EffectSpec.GetAllAssetTags(DamageTags);

	const bool bCannotBeParried = DamageTags.HasTag(CXVIGameplayTags::Damage_CannotBeParried);

	AActor* Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();

	// 패링 우선. 회피 태그가 같이 있어도 패링 성공이면 회피 알림은 보내지 않음
	if (IncomingDamage > 0.f && !bCannotBeParried &&
		TargetASC->HasMatchingGameplayTag(CXVIGameplayTags::State_Combat_Parrying))
	{
		SetAP(FMath::Clamp(GetAP() + 1.f, 0.f, GetMaxAP()));

		OnParrySucceeded.Broadcast(Instigator);

		return true;
	}

	if (TargetASC->HasMatchingGameplayTag(CXVIGameplayTags::State_Combat_Dodging))
	{
		UE_LOG(LogTemp, Display, TEXT("%s 회피 성공: 데미지 %.1f 무시"), *GetNameSafe(GetOwningActor()), IncomingDamage);

		OnDodgeSucceeded.Broadcast(Instigator);

		return true;
	}

	return Result;
}
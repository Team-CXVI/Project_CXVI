// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/CXCombatantViewModel.h"

#include "AbilitySystem/Attributes/Combat/CXCombatAttributeSet.h"

#include <AbilitySystemComponent.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatantViewModel)

void UCXCombatantViewModel::Bind(UAbilitySystemComponent* InASC)
{
	Unbind();

	if (nullptr == InASC)
	{
		return;
	}

	BoundASC = InASC;

	BindAttribute(UCXCombatAttributeSet::GetMaxHealthAttribute(), &ThisClass::SetMaxHealth);
	BindAttribute(UCXCombatAttributeSet::GetHealthAttribute(), &ThisClass::SetHealth);
	BindAttribute(UCXCombatAttributeSet::GetMaxManaAttribute(), &ThisClass::SetMaxAP);
	BindAttribute(UCXCombatAttributeSet::GetManaAttribute(), &ThisClass::SetAP);
}

void UCXCombatantViewModel::Unbind()
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		for (const TPair<FGameplayAttribute, FDelegateHandle>& Pair : AttributeHandles)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Pair.Key).Remove(Pair.Value);
		}
	}

	AttributeHandles.Reset();
	BoundASC.Reset();
}

float UCXCombatantViewModel::GetHealthPercent() const
{
	return MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f;
}

bool UCXCombatantViewModel::IsDead() const
{
	// 초기화 전(MaxHealth 0)은 사망으로 보지 않음
	return MaxHealth > 0.f && Health <= 0.f;
}

void UCXCombatantViewModel::BindAttribute(const FGameplayAttribute& InAttribute, void(ThisClass::* InSetter)(float))
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (nullptr == ASC)
	{
		return;
	}

	(this->*InSetter)(ASC->GetNumericAttribute(InAttribute));

	const FDelegateHandle Handle = ASC->GetGameplayAttributeValueChangeDelegate(InAttribute).AddWeakLambda(this, [this, InSetter](const FOnAttributeChangeData& InData)
		{
			(this->*InSetter)(InData.NewValue);
		});

	AttributeHandles.Emplace(InAttribute, Handle);
}

void UCXCombatantViewModel::SetHealth(float InValue)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(Health, InValue))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthPercent);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsDead);
	}
}

void UCXCombatantViewModel::SetMaxHealth(float InValue)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(MaxHealth, InValue))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthPercent);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsDead);
	}
}

void UCXCombatantViewModel::SetAP(float InValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(AP, InValue);
}

void UCXCombatantViewModel::SetMaxAP(float InValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(MaxAP, InValue);
}

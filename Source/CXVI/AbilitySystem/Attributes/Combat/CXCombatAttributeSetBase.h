// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"

#include "AbilitySystemComponent.h"

#include "CXCombatAttributeSetBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCXAttributeDataChanged, float, OldValue, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCXOutOfHealth, AActor*, InInstigator);
DECLARE_MULTICAST_DELEGATE_TwoParams(FCXDamageApplied, AActor*, float);
DECLARE_MULTICAST_DELEGATE_OneParam(FCXParrySucceeded, AActor*);

/**
 * 
 */
UCLASS()
class CXVI_API UCXCombatAttributeSetBase : public UAttributeSet
{
	GENERATED_BODY()

public:
	UCXCombatAttributeSetBase();

	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, MaxHealth);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, Health);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, Damage);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, CombatSpeed);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, AttackPower);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, Defense);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, CriticalChance);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, CriticalDamageMultiplier);

protected:
	// 현재 적용값이 바뀌기 직전
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	// 현재 적용값이 바뀐 직후
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	// 기준값이 바뀌기 직전
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	// GE가 Attribute의 기준값을 직접 변경한 직후
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual bool TryAvoidDamage(const FGameplayEffectModCallbackData& Data, float IncomingDamage);

	float CalculateFinalDamage(const FGameplayEffectModCallbackData& Data, float IncomingDamage) const;

public:
	UPROPERTY(BlueprintAssignable, Category = "Attribute")
	mutable FCXOutOfHealth OnOutOfHealth;

	mutable FCXDamageApplied OnDamageApplied;

	FCXParrySucceeded OnParrySucceeded;

protected:
	UPROPERTY(BlueprintAssignable, Category = "Attribute")
	mutable FCXAttributeDataChanged OnMaxHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attribute")
	mutable FCXAttributeDataChanged OnHealthChanged;

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Health;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Damage;

	UPROPERTY(BlueprintReadOnly, Category = "Attirbutes")
	FGameplayAttributeData CombatSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Attirbutes")
	FGameplayAttributeData AttackPower;

	UPROPERTY(BlueprintReadOnly, Category = "Attirbutes")
	FGameplayAttributeData Defense;

	UPROPERTY(BlueprintReadOnly, Category = "Attirbutes")
	FGameplayAttributeData CriticalChance;

	UPROPERTY(BlueprintReadOnly, Category = "Attirbutes")
	FGameplayAttributeData CriticalDamageMultiplier;


};

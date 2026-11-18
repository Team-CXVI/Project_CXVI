// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"

#include "AbilitySystemComponent.h"

#include "CXCombatAttributeSet.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAttributeDataChanged_V2, float, OldValue, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOutOfHealth_V2, AActor*, InInstigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDamageApplied_V2, AActor*, InInstigator, float, AppliedDamage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCXParrySucceeded_V2, AActor*, InInstigator);


/**
 * 
 */
UCLASS()
class CXVI_API UCXCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UCXCombatAttributeSet();

	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, MaxHealth);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, Health);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, Damage);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, AP);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, MaxAP);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, CombatSpeed);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, AttackRange);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, AttackRadius);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, AttackRate);

protected:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

public:
	UPROPERTY(BlueprintAssignable, Category = "Attribute")
	mutable FOutOfHealth_V2 OnOutOfHealth;

	UPROPERTY(BlueprintAssignable, Category = "Attribute")
	mutable FDamageApplied_V2 OnDamageApplied;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Parry")
	FCXParrySucceeded_V2 OnParrySucceeded;

protected:

	UPROPERTY(BlueprintAssignable, Category = "Attribute")
	mutable FAttributeDataChanged_V2 OnMaxHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attribute")
	mutable FAttributeDataChanged_V2 OnHealthChanged;

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Health;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Damage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attirbutes")
	FGameplayAttributeData AP;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attirbutes")
	FGameplayAttributeData MaxAP;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attirbutes")
	FGameplayAttributeData CombatSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attirbutes")
	FGameplayAttributeData AttackRange;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attirbutes")
	FGameplayAttributeData AttackRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attirbutes")
	FGameplayAttributeData AttackRate;

};

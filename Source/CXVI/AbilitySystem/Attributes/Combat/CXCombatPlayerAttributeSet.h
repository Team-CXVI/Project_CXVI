// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/Combat/CXCombatAttributeSetBase.h"
#include "CXCombatPlayerAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXCombatPlayerAttributeSet : public UCXCombatAttributeSetBase
{
	GENERATED_BODY()
	
public:
	UCXCombatPlayerAttributeSet();

	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, AP);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, MaxAP);

protected:
	// 현재 적용값이 바뀌기 직전
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	// 현재 적용값이 바뀐 직후
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	// 기준값이 바뀌기 직전
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	// GE가 Attribute의 기준값을 직접 변경한 직후
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual bool TryAvoidDamage(const FGameplayEffectModCallbackData& Data, float IncomingDamage) override;

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData AP;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxAP;

};

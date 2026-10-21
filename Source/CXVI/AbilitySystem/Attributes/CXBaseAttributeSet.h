// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"

#include "CXBaseAttributeSet.generated.h"

// 델리게이트 선언.
DECLARE_MULTICAST_DELEGATE(FCXOnHpZeroDelegate);
DECLARE_MULTICAST_DELEGATE(FCXOnStatChangedDelegate);
/**
 * 
 */
UCLASS()
class CXVI_API UCXBaseAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UCXBaseAttributeSet, MaxHealth);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UCXBaseAttributeSet, Health);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS_BASIC(UCXBaseAttributeSet, MaxMana);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS_BASIC(UCXBaseAttributeSet, Mana);

	// Attribute의 변경이 확정되기 전 클램프 처리, HP가 Max치 보다 높거나 0보다 작아지는 걸 막는 처리를 진행.
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	// GamePlayEffect가 적용된 후 처리.
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	FCXOnHpZeroDelegate OnHpZero;
	FCXOnStatChangedDelegate OnStatChanged;

	
};

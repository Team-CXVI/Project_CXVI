// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"

#include "AbilitySystemComponent.h"

#include "CXCombatAttributeSetBase.generated.h"

/**
 * OnDamageApplied로 전달하는 피격 정보
 * - 실제로 피해를 입었을 때만 전달 (회피 / 패링은 각자 델리게이트)
 * - 피격 처리에 필요한 정보가 늘어나면 여기에 필드 추가
 */
USTRUCT(BlueprintType)
struct CXVI_API FCXDamageAppliedData
{
	GENERATED_BODY()

public:
	// 공격한 액터
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TObjectPtr<AActor> Instigator = nullptr;

	// 치명타, 방어력까지 반영한 최종 피해
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float Amount = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsCritical = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCXAttributeDataChanged, float, OldValue, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCXOutOfHealth, AActor*, InInstigator);
DECLARE_MULTICAST_DELEGATE_OneParam(FCXDamageApplied, const FCXDamageAppliedData&);
DECLARE_MULTICAST_DELEGATE_OneParam(FCXParrySucceeded, AActor*);
DECLARE_MULTICAST_DELEGATE_OneParam(FCXDodgeSucceeded, AActor*);

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

	// 치명타, 방어력을 반영한 최종 피해. 치명타가 발생하면 bOutCritical = true
	float CalculateFinalDamage(const FGameplayEffectModCallbackData& Data, float IncomingDamage, bool& bOutCritical) const;

public:
	UPROPERTY(BlueprintAssignable, Category = "Attribute")
	mutable FCXOutOfHealth OnOutOfHealth;

	// 실제로 피해를 입었을 때 (회피 / 패링 성공 시에는 호출 안 됨)
	mutable FCXDamageApplied OnDamageApplied;

	// 패링 성공 (인자 = 공격자)
	FCXParrySucceeded OnParrySucceeded;

	// 회피 성공 (인자 = 공격자)
	FCXDodgeSucceeded OnDodgeSucceeded;

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

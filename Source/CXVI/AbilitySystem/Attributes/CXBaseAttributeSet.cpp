// Fill out your copyright notice in the Description page of Project Settings.


#include "CXBaseAttributeSet.h"
#include "GameplayEffectExtension.h"

void UCXBaseAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
	}
}

void UCXBaseAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
		OnStatChanged.Broadcast();
		if (GetHealth() <= 0.0f)
		{
			AActor* OwnerActor = GetOwningActor();
			// HasAuthority의 경우 실행 주최를 의미하는데 권위 서버가 붙어있는 경우 서버에서만 처리할 수 있도록 하는 체크.
			if (OwnerActor && OwnerActor->HasAuthority())
			{
				OnHpZero.Broadcast();
			}
		}
	}
}

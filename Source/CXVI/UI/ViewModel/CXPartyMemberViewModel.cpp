// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/CXPartyMemberViewModel.h"

#include "Foundation/UserData/CXUserDataTypes.h"
#include "Foundation/UserData/CXCharacterDefinition.h"

#include <Engine/Texture2D.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXPartyMemberViewModel)

void UCXPartyMemberViewModel::Refresh(const FCXPartyMemberState& InMember, const UCXCharacterDefinition* InDefinition, float InMaxHealth)
{
	SetPortrait(InDefinition ? InDefinition->Portrait : TSoftObjectPtr<UTexture2D>());
	SetLevel(InMember.Level);
	SetMaxHealth(InMaxHealth);
	SetHealth(InMember.CurrentHealth);
}

FText UCXPartyMemberViewModel::GetHealthText() const
{
	return FText::Format(INVTEXT("{0} / {1}"), FText::AsNumber(FMath::RoundToInt(Health)), FText::AsNumber(FMath::RoundToInt(MaxHealth)));
}

float UCXPartyMemberViewModel::GetHealthPercent() const
{
	return MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f;
}

void UCXPartyMemberViewModel::SetPortrait(const TSoftObjectPtr<UTexture2D>& InValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(Portrait, InValue);
}

void UCXPartyMemberViewModel::SetLevel(int32 InValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(Level, InValue);
}

void UCXPartyMemberViewModel::SetHealth(float InValue)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(Health, InValue))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthPercent);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthText);
	}
}

void UCXPartyMemberViewModel::SetMaxHealth(float InValue)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(MaxHealth, InValue))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthPercent);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthText);
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Foundation/GameplayData/Types/CXItemTypes.h"

#include "Tags/CXGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXItemTypes)

bool FCXItemRow::TryGetRecoveryValue(int32 InItemLevel, float& OutValue) const
{
	// 0번 칸이 강화 레벨 1
	const int32 RecoveryIndex = InItemLevel - 1;
	if (false == RecoveryValues.IsValidIndex(RecoveryIndex) || false == FMath::IsFinite(RecoveryValues[RecoveryIndex]))
	{
		return false;
	}

	OutValue = RecoveryValues[RecoveryIndex];
	return true;
}

FText FCXItemRow::FormatDescription(int32 InItemLevel) const
{
	float RecoveryValue = 0.f;
	if (false == TryGetRecoveryValue(InItemLevel, RecoveryValue))
	{
		return Description;
	}

	// 회복량 단위 해석은 ACXCombatCharacterPlayer::UseItem과 같은 기준
	int32 DisplayValue = 0;
	if (ItemType.MatchesTagExact(CXVIGameplayTags::Item_Type_HealingTint))
	{
		DisplayValue = FMath::RoundToInt(RecoveryValue * 100.f);
	}
	else if (ItemType.MatchesTagExact(CXVIGameplayTags::Item_Type_EnergyTint))
	{
		DisplayValue = FMath::RoundToInt(RecoveryValue);
	}
	else
	{
		return Description;
	}

	return FText::FormatNamed(Description, TEXT("Recovery"), FText::AsNumber(DisplayValue));
}

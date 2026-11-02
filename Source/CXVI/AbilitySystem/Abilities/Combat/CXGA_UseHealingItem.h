// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Combat/CXGA_UseItemBase.h"
#include "CXGA_UseHealingItem.generated.h"

class ACXCombatCharacterPlayer;
class ACXCombatCharacterBase;

/**
 * 
 */
UCLASS()
class CXVI_API UCXGA_UseHealingItem : public UCXGA_UseItemBase
{
	GENERATED_BODY()

public:
	UCXGA_UseHealingItem();

protected:
	virtual bool IsItemDataValid(const FCXItemRow& ItemRow) const override;

	virtual bool IsItemTargetValid(
		const ACXCombatCharacterPlayer* User,
		const ACXCombatCharacterBase* Target) const override;

	virtual bool ApplyItemEffect(
		ACXCombatCharacterPlayer* User,
		ACXCombatCharacterBase* Target,
		const FCXItemRow& ItemRow) override;

protected:
	// 에디터에서 만든 공통 회복 GE지정
	UPROPERTY(EditDefaultsOnly, Category = "Item|Recovery")
	TSubclassOf<class UGameplayEffect> HealingEffectClass;

};

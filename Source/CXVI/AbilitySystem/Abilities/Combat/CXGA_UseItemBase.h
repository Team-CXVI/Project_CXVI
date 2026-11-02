// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CXGA_UseItemBase.generated.h"

class ACXCombatCharacterPlayer;
class ACXCombatCharacterBase;
struct FCXItemRow;

/**
 * 
 */
UCLASS(Abstract)
class CXVI_API UCXGA_UseItemBase : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UCXGA_UseItemBase();

	//  Target이 없으면 아이템 선택 단계의 검사만 수행
	bool CanUseItem(const ACXCombatCharacterPlayer* User, FName ItemId, const ACXCombatCharacterBase* Target = nullptr) const;

protected:
    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

    // 아래 세 함수는 아이템 종류별 자식이 구현
    virtual bool IsItemDataValid(const FCXItemRow& ItemRow) const
    {
        return false;
    }

    virtual bool IsItemTargetValid(const ACXCombatCharacterPlayer* User, const ACXCombatCharacterBase* Target) const
    {
        return false;
    }

    // 실제 효과가 적용됐을 때 true 반환
    virtual bool ApplyItemEffect(ACXCombatCharacterPlayer* User, ACXCombatCharacterBase* Target, const FCXItemRow& ItemRow)
    {
        return false;
    }

private:
    static const FCXItemRow* FindItemRow(const ACXCombatCharacterPlayer* User, FName ItemId);
};

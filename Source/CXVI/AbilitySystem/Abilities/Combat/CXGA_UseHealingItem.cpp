// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Combat/CXGA_UseHealingItem.h"

#include "AbilitySystem/Attributes/Combat/CXCombatAttributeSetBase.h"
#include "Character/Combat/CXCombatCharacterBase.h"
#include "Character/Combat/CXCombatCharacterPlayer.h"
#include "Foundation/GameplayData/Types/CXItemTypes.h"
#include "Tags/CXGameplayTags.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGA_UseHealingItem)


UCXGA_UseHealingItem::UCXGA_UseHealingItem()
{
    FGameplayTagContainer Tags = GetAssetTags();
    Tags.AddTag(CXVIGameplayTags::Ability_Item_Heal);
    SetAssetTags(Tags);
}

bool UCXGA_UseHealingItem::IsItemDataValid(const FCXItemRow& ItemRow) const
{
    return HealingEffectClass.Get() != nullptr;
}

bool UCXGA_UseHealingItem::IsItemTargetValid(const ACXCombatCharacterPlayer* User, const ACXCombatCharacterBase* Target) const
{
    if (!IsValid(Target) || !Cast<ACXCombatCharacterPlayer>(Target) || !Target->GetIsCharacterAlive())
    {
        return false;
    }

    const UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();

    const UCXCombatAttributeSetBase* Attributes = TargetASC
        ? TargetASC->GetSet<UCXCombatAttributeSetBase>() : nullptr;

    return Attributes && Attributes->GetHealth() > 0.f && Attributes->GetHealth() < Attributes->GetMaxHealth();
}

bool UCXGA_UseHealingItem::ApplyItemEffect(ACXCombatCharacterPlayer* User, ACXCombatCharacterBase* Target, const FCXItemRow& ItemRow)
{
    if (!IsValid(User) || !IsValid(Target))
    {
        return false;
    }

    UAbilitySystemComponent* SourceASC = User->GetAbilitySystemComponent();

    UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();

    if (!IsValid(SourceASC) || !IsValid(TargetASC))
    {
        return false;
    }

    const float ItemEffectLevel = 1.f;

    FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(HealingEffectClass, ItemEffectLevel);

    //const float MaxHealth = TargetASC->GetNumericAttribute(UCXCombatAttributeSetBase::GetMaxHealthAttribute());

    //const float HealAmount = MaxHealth * FMath::Clamp(ItemRow.HelathRestoreRatio, 0.f, 1.f);

    //if (HealAmount <= 0.f)
    //{
    //    return false;
    //}

    //FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(HealingEffectClass, GetAbilityLevel());

    if (!Spec.IsValid())
    {
        return false;
    }

    const float OldHealth = TargetASC->GetNumericAttribute(UCXCombatAttributeSetBase::GetHealthAttribute());

    //Spec.Data->SetSetByCallerMagnitude(CXVIGameplayTags::Data_Heal, HealAmount);


    SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);

    if (!IsValid(TargetASC))
    {
        return false;
    }

    const float NewHealth = TargetASC->GetNumericAttribute(UCXCombatAttributeSetBase::GetHealthAttribute());

    const float AppliedHealing = FMath::Max(0.f, NewHealth - OldHealth);

    if (AppliedHealing <= 0.f)
    {
        return false;
    }

    UE_LOG(LogTemp, Display, TEXT("[회복 아이템] %s -> %s, 효과 레벨=%.0f, 실제 회복=%.1f, HP %.1f -> %.1f"),
        *GetNameSafe(User),
        *GetNameSafe(Target),
        ItemEffectLevel,
        AppliedHealing,
        OldHealth,
        NewHealth
    );

    return true;
}

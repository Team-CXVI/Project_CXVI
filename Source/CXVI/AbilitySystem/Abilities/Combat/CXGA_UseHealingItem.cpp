// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Combat/CXGA_UseHealingItem.h"

#include "AbilitySystem/Attributes/Combat/CXCombatAttributeSetBase.h"
#include "Character/Combat/CXCombatCharacterBase.h"
#include "Character/Combat/CXCombatCharacterPlayer.h"
#include "Foundation/GameplayData/CXGameplayDataSubsystem.h"
#include "Foundation/UserData/CXUserDataSubsystem.h"
#include "Tags/CXGameplayTags.h"

#include "AbilitySystemComponent.h"
#include "Engine/GameInstance.h"
#include "GameplayEffect.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGA_UseHealingItem)

UCXGA_UseHealingItem::UCXGA_UseHealingItem()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UCXGA_UseHealingItem::CanUseHealingItem(const ACXCombatCharacterPlayer* User, FName ItemId, const ACXCombatCharacterBase* Target)
{
    if (!IsValid(User) || !User->GetIsCharacterAlive() || !User->GetIsTurn())
    {
        return;
    }

    const UGameInstance* GameInstance = User->GetGameInstance();

    const UCXGameplayDataSubsystem* GameplayData = GameInstance ?
        GameInstance->GetSubsystem<UCXGameplayDataSubsystem>() : nullptr;

    const UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(User);

    const FCXItemRow* ItemRow = GameplayData ? GameplayData->FindItemRow(ItemId) : nullptr;

    if (ItemId.IsNone() || !ItemRow || ItemRow->HelathRestoreRatio <= 0.f)
    {
        UE_LOG(LogTemp, Display, TEXT("[회복 아이템] 아이템 정보 또는 회복량 확인: %s"), *ItemId.ToString());

        return false;
    }

    if (!UserData || UserData->GetItemCount(ItemId) <= 0)
    {
        UE_LOG(LogTemp, Display, TEXT("[회복 아이템] 아이템 보유 수량 부족: %s"), *ItemId.ToString());

        return false;
    }

    // 메뉴에서 아이템만 선택하는단계
    if (!Target)
    {
        return true;
    }

    // 플레이어 팀의 살아있는 캐릭터만 허용
    if (!IsValid(Target) || !Cast<ACXCombatCharacterPlayer>(Target) || !Target->GetIsCharacterAlive())
    {
        UE_LOG(LogTemp, Display, TEXT("[회복 아이템] 살아있는 아군을 선택해야만 합니다."));

        return false;
    }

    const UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();

    const UCXCombatAttributeSetBase* Attributes = TargetASC ? TargetASC->GetSet<UCXCombatAttributeSetBase>() : nullptr;

    if (!Attributes)
    {
        return false;
    }

    if (Attributes->GetHealth() >= Attributes->GetMaxHealth())
    {
        UE_LOG(LogTemp, Display, TEXT("[회복 아이템] %s : 회복할 체력이 없습니다. 최대체력임."), *Target->GetName());

        return false;
    }

    return true;
}

void UCXGA_UseHealingItem::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo, 
    const FGameplayAbilityActivationInfo ActivationInfo, 
    const FGameplayEventData* TriggerEventData)
{
    ACXCombatCharacterPlayer* User = ActorInfo ? Cast<ACXCombatCharacterPlayer>(ActorInfo->AvatarActor.Get()) : nullptr;

    if (!IsValid(User) || !User->HasAuthority())
    {
        return;
    }

    const FName ItemId = User->GetSelectedItemId();

    ACXCombatCharacterBase* Target = Cast<ACXCombatCharacterBase>(User->GetSelectedCombatTarget());

    if (!IsValid(Target) || !CanUseHealingItem(User, ItemId, Target) || !HealingEffectClass)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);

        return;
    }

    UAbilitySystemComponent* SourceASC = User->GetAbilitySystemComponent();

    UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();

    UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(User);

    const UGameInstance* GameInstance = User->GetGameInstance();

    const UCXGameplayDataSubsystem* GameplayData = GameInstance ?
        GameInstance->GetSubsystem<UCXGameplayDataSubsystem>() : nullptr;

    const FCXItemRow* ItemRow = GameplayData ? GameplayData->FindItemRow(ItemId) : nullptr;

    if (!IsValid(SourceASC) || !IsValid(TargetASC) || !UserData || !ItemRow)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
     
        return;
    }

    const float HealRatio = ItemRow->HelathRestoreRatio;

    FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(
        Handle,
        ActorInfo,
        ActivationInfo,
        HealingEffectClass,
        GetAbilityLevel(Handle, ActorInfo)
    );

    if (!Spec.IsValid())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);

        return;
    }

    Spec.Data->SetSetByCallerMagnitude(CXVIGameplayTags::Data_Heal, HealRatio);

    // 이번 GA는 쿨다운이 없음
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);

        return;
    }

    // 실행 시점에도 실제 수량 차감 성공 여부 확인.
    if (!UserData->RemoveItem(ItemId, 1))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        
        return;
    }

    const float OldHealth = TargetASC->GetNumericAttribute(UCXCombatAttributeSetBase::GetHealthAttribute());

    SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);

    const float NewHealth = TargetASC->GetNumericAttribute(UCXCombatAttributeSetBase::GetHealthAttribute());

    const float AppliedHealing = FMath::Max(0.f, NewHealth - OldHealth);

    // Instant GE는 반환 핸들의 유효성으로 성공을 판단하지 않음.
    // 실제 HP가 증가하지 않았다면 소비한 아이템 복구
    if (AppliedHealing <= 0.f)
    {
        UserData->AddItem(ItemId, 1);

        UE_LOG(LogTemp, Warning, TEXT("[회복 아이템] 회복 미적용: 수량 복구 Item=%s"), *ItemId.ToString());

        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);

        return;
    }

    UE_LOG(LogTemp, Display, TEXT("[회복 아이템] %s -> %s, Item=%s, 요청=%.1f, 실제=%.1f, HP %.1f → %.1f, 남은 수량=%d"),
        *User->GetName(),
        *Target->GetName(),
        *ItemId.ToString(),
        HealAmount,
        AppliedHealing,
        OldHealth,
        NewHealth,
        UserData->GetItemCount(ItemId)
    );

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UCXGA_UseHealingItem::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo, 
    const FGameplayTagContainer* SourceTags, 
    const FGameplayTagContainer* TargetTags, 
    OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!ActorInfo || !Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }

    if (!HealingEffectClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[회복 아이템] HealingEffectClass가 없습니다."));

        return false;
    }

    const ACXCombatCharacterPlayer* User = Cast<ACXCombatCharacterPlayer>(ActorInfo->AvatarActor.Get());

    if (!IsValid(User))
    {
        return false;
    }

    return CanUseHealingItem(User, User->GetSelectedItemId());
}

void UCXGA_UseHealingItem::EndAbility(
    const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo, 
    const FGameplayAbilityActivationInfo ActivationInfo, 
    bool bReplicateEndAbility, 
    bool bWasCancelled)
{
    if (!IsEndAbilityValid(Handle, ActorInfo))
    {
        return;
    }

    ACXCombatCharacterBase* User = ActorInfo ?
        Cast<ACXCombatCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

    if (IsValid(User))
    {
        User->NotifyBattleAbilityEnded(Handle, bWasCancelled);
    }
}

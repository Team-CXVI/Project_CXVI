// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Combat/CXGA_UseItemBase.h"

#include "Character/Combat/CXCombatCharacterBase.h"
#include "Character/Combat/CXCombatCharacterPlayer.h"
#include "Foundation/GameplayData/CXGameplayDataSubsystem.h"
#include "Foundation/UserData/CXUserDataSubsystem.h"

#include "Engine/GameInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGA_UseItemBase)

UCXGA_UseItemBase::UCXGA_UseItemBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UCXGA_UseItemBase::CanUseItem(const ACXCombatCharacterPlayer* User, FName ItemId, const ACXCombatCharacterBase* Target) const
{
	if (!IsValid(User) || !User->GetIsCharacterAlive() || !User->GetIsTurn())
	{
		return false;
	}

	const FCXItemRow* ItemRow = FindItemRow(User, ItemId);

	if (!ItemRow || !IsItemDataValid(*ItemRow))
	{
		UE_LOG(LogTemp, Display, TEXT("[아이템] 아이템 정보,효과 설정 확인: %s"), *ItemId.ToString());

		return false;
	}

	const UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(User);

	if (!UserData || UserData->GetItemCount(ItemId) <= 0)
	{
		UE_LOG(LogTemp, Display, TEXT("[아이템] 보유 수량 부족: %s"), *ItemId.ToString());

		return false;
	}

	// 메뉴에서 아이템만 고르는 단계에서는 대상 검사를 생략
	if (!Target)
	{
		return true;
	}

	if (!IsItemTargetValid(User, Target))
	{
		UE_LOG(LogTemp, Display, TEXT("[아이템] 사용할 수 없는 대상 :%s "), *GetNameSafe(Target));

		return false;
	}

	return true;

}

bool UCXGA_UseItemBase::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ActorInfo || !Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const ACXCombatCharacterPlayer* User = Cast<ACXCombatCharacterPlayer>(ActorInfo->AvatarActor.Get());

	if (!IsValid(User))
	{
		return false;
	}

	const ACXCombatCharacterBase* Target = Cast<ACXCombatCharacterBase>(User->GetSelectedCombatTarget());

	// 실제 활성화 시점에는 대상이 반드시 필요
	return IsValid(Target) && CanUseItem(User, User->GetSelectedItemId(), Target);
}

void UCXGA_UseItemBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ACXCombatCharacterPlayer* User = ActorInfo ? Cast<ACXCombatCharacterPlayer>(ActorInfo->AvatarActor.Get()) : nullptr;

	if (!IsValid(User) || !User->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);

		return;
	}

	const FName ItemId = User->GetSelectedItemId();

	ACXCombatCharacterBase* Target = Cast<ACXCombatCharacterBase>(User->GetSelectedCombatTarget());

	if (!IsValid(Target) || !CanUseItem(User, ItemId, Target))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);

		return;
	}

	UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(User);

	const FCXItemRow* ItemRow = FindItemRow(User, ItemId);

	if (!IsValid(UserData) || !ItemRow)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);

		return;
	}

	// 수량 변경 메시지 등이 실행돼도 이번 효과데이터는 유지
	const FCXItemRow ItemData = *ItemRow;

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);

		return;
	}

	// 아이템 수량은 부모의 이 지점에서만 소비
	if (!UserData->RemoveItem(ItemId, 1))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);

		return;
	}

	// 실제 효과는 종류별 자식에서 처리
	if (!ApplyItemEffect(User, Target, ItemData))
	{
		if (IsValid(UserData))
		{
			UserData->AddItem(ItemId, 1);
		}

		UE_LOG(LogTemp, Warning, TEXT("[아이템] 효과 미적용: 수량 복구 Item=%s"), *ItemId.ToString());

		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);

		return;
	}

	UE_LOG(LogTemp, Display, TEXT("[아이템] 사용 완료:Item=%s, 남은 수량=%d"), *ItemId.ToString(), IsValid(UserData) ? UserData->GetItemCount(ItemId) : 0);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UCXGA_UseItemBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!ActorInfo || !IsEndAbilityValid(Handle, ActorInfo))
	{
		return;
	}

	ACXCombatCharacterBase* User = Cast<ACXCombatCharacterBase>(ActorInfo->AvatarActor.Get());

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	if (IsValid(User))
	{
		User->NotifyBattleAbilityEnded(Handle, bWasCancelled);
	}

}

const FCXItemRow* UCXGA_UseItemBase::FindItemRow(const ACXCombatCharacterPlayer* User, FName ItemId)
{
	if (!IsValid(User) || ItemId.IsNone())
	{
		return nullptr;
	}

	const UGameInstance* GameInstance = User->GetGameInstance();

	const UCXGameplayDataSubsystem* GameplayData = GameInstance
		? GameInstance->GetSubsystem<UCXGameplayDataSubsystem>() : nullptr;

	return GameplayData ? GameplayData->FindItemRow(ItemId) : nullptr;
}

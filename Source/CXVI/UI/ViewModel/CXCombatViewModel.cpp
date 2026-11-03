// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/CXCombatViewModel.h"

#include "CXCombatantViewModel.h"
#include "Character/Combat/CXCombatCharacterBase.h"
#include "Tags/CXGameplayTags.h"
#include "Foundation/Combat/CXCombatManagerSubsystem.h"
#include "Foundation/Combat/CombatMessage/CXCombatMessageData.h"
#include "Foundation/UserData/CXCharacterDefinition.h"
#include "Foundation/UserData/CXUserDataSubsystem.h"

// TODO : CXLOG Category 정리 후 제거
#include "UI/CXUIManagerSubsystem.h"

#include <AbilitySystemComponent.h>
#include <AbilitySystemGlobals.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatViewModel)

void UCXCombatViewModel::SetCombatants(const TArray<TObjectPtr<ACXCombatCharacterBase>>& InPlayers, const ACXCombatCharacterBase* InEnemy)
{
	Reset();

	// 첫 턴 시작(BeginBattle)보다 먼저 구독
	TurnOrderChangedListener = UCXGameplayMessageSubsystem::Get(this).RegisterListener(CXVIGameplayTags::Message_Combat_TurnOrderChanged, this, &ThisClass::HandleTurnOrderChanged);

	for (ACXCombatCharacterBase* Player : InPlayers)
	{
		if (UCXCombatantViewModel* ViewModel = CreateCombatantViewModel(Player))
		{
			Players.Emplace(ViewModel);
		}
	}

	if (nullptr != InEnemy)
	{
		const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InEnemy);
		if (ASC && ASC->HasMatchingGameplayTag(CXVIGameplayTags::Character_Type_Boss))
		{
			Boss = CreateCombatantViewModel(InEnemy);
		}
	}

	BroadcastAll();
}

void UCXCombatViewModel::Reset()
{
	TurnOrderChangedListener.Unregister();

	for (UCXCombatantViewModel* ViewModel : Players)
	{
		ViewModel->Unbind();
	}

	if (Boss)
	{
		Boss->Unbind();
	}

	Players.Reset();
	Boss = nullptr;
	TurnOrder.Reset();

	BroadcastAll();
}

UCXCombatantViewModel* UCXCombatViewModel::CreateCombatantViewModel(const AActor* InCombatant)
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InCombatant);
	if (nullptr == ASC)
	{
		UE_LOG(LogCXUI, Warning, TEXT("CombatViewModel : %s에 ASC가 없어 HUD에 표시하지 않습니다."), *GetNameSafe(InCombatant));
		return nullptr;
	}

	UCXCombatantViewModel* ViewModel = NewObject<UCXCombatantViewModel>(this);
	ViewModel->Bind(ASC);
	return ViewModel;
}

void UCXCombatViewModel::BroadcastAll()
{
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Players);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Boss);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(HasBoss);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TurnOrder);
}

void UCXCombatViewModel::HandleTurnOrderChanged(const FGameplayTag& InChannel, const FCXTurnOrderChangedMessage& InMessage)
{
	RefreshTurnOrder();
}

void UCXCombatViewModel::RefreshTurnOrder()
{
	const UCXCombatManagerSubsystem* CombatManager = UCXCombatManagerSubsystem::TryGet(this);
	if (nullptr == CombatManager)
	{
		return;
	}

	TurnOrder.Reset();

	for (const TWeakObjectPtr<ACXCombatCharacterBase>& WeakCharacter : CombatManager->GetUpcomingTurns())
	{
		const ACXCombatCharacterBase* Character = WeakCharacter.Get();
		if (nullptr == Character)
		{
			continue;
		}

		FCXTurnOrderEntryData& Entry = TurnOrder.AddDefaulted_GetRef();
		Entry.Character = WeakCharacter;
		Entry.Portrait = FindPortrait(*Character);
	}

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TurnOrder);
}

TSoftObjectPtr<UTexture2D> UCXCombatViewModel::FindPortrait(const ACXCombatCharacterBase& InCharacter) const
{
	// TODO : 적 전용 데이터가 정해지면 적은 여기서 분기
	const FPrimaryAssetId CharacterId = InCharacter.GetCharacterId();
	if (!CharacterId.IsValid())
	{
		return TSoftObjectPtr<UTexture2D>();
	}

	const UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this);
	const UCXCharacterDefinition* Definition = UserData ? UserData->LoadCharacterDefinition(CharacterId) : nullptr;

	return Definition ? Definition->Portrait : TSoftObjectPtr<UTexture2D>();
}

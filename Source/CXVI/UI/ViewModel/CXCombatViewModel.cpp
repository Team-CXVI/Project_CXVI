// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/CXCombatViewModel.h"

#include "CXCombatantViewModel.h"
#include "Character/Combat/CXCombatCharacterBase.h"
#include "Tags/CXGameplayTags.h"
#include "Foundation/Combat/CXCombatManagerSubsystem.h"
#include "Foundation/Combat/CombatMessage/CXCombatMessageData.h"
#include "Foundation/UserData/CXCharacterDefinition.h"
#include "Foundation/UserData/CXUserDataSubsystem.h"
#include "Character/Combat/CXCombatCharacterEnemy.h"

// TODO : CXLOG Category 정리 후 제거
#include "UI/CXUIManagerSubsystem.h"

#include <AbilitySystemComponent.h>
#include <AbilitySystemGlobals.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatViewModel)

void UCXCombatViewModel::SetCombatants(const TArray<TObjectPtr<ACXCombatCharacterBase>>& InPlayers, const ACXCombatCharacterEnemy* InBoss)
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

	if (nullptr != InBoss)
	{
		Boss = CreateCombatantViewModel(InBoss);
		if (Boss)
		{
			Boss->SetDisplayName(InBoss->GetEnemyDisplayName());
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

	if (const ACXCombatCharacterBase* Character = Cast<ACXCombatCharacterBase>(InCombatant))
	{
		ViewModel->SetPortrait(FindPortrait(*Character));
	}

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
	// 스폰 시 정의 에셋에서 받은 초상화 (파티원 / 적 공통)
	if (!InCharacter.GetPortrait().IsNull())
	{
		return InCharacter.GetPortrait();
	}

	// 스폰 정보 없이 생성된 경우(클래스 직접 지정 테스트 등) BP에 지정된 CharacterId로 조회
	const FPrimaryAssetId CharacterId = InCharacter.GetCharacterId();
	if (!CharacterId.IsValid())
	{
		return TSoftObjectPtr<UTexture2D>();
	}

	const UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this);
	const UCXCharacterDefinition* Definition = UserData ? UserData->LoadCharacterDefinition(CharacterId) : nullptr;

	return Definition ? Definition->Portrait : TSoftObjectPtr<UTexture2D>();
}

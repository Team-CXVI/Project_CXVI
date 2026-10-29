// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/CXCombatViewModel.h"

#include "CXCombatantViewModel.h"
#include "Character/Combat/CXCombatCharacterBase.h"
#include "Tags/CXGameplayTags.h"

// TODO : CXLOG Category 정리 후 제거
#include "UI/CXUIManagerSubsystem.h"

#include <AbilitySystemComponent.h>
#include <AbilitySystemGlobals.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatViewModel)

void UCXCombatViewModel::SetCombatants(const TArray<TObjectPtr<ACXCombatCharacterBase>>& InPlayers, const TObjectPtr<ACXCombatCharacterBase>& InEnemy)
{
	Reset();

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

	BroadcastAll();
}

UCXCombatantViewModel* UCXCombatViewModel::CreateCombatantViewModel(AActor* InCombatant)
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InCombatant);
	if (nullptr == ASC)
	{
		UE_LOG(LogCXUI, Warning, TEXT("BattleViewModel : %s에 ASC가 없어 HUD에 표시하지 않습니다."), *GetNameSafe(InCombatant));
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
}

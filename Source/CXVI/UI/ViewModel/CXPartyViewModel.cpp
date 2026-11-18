// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/CXPartyViewModel.h"

#include "CXPartyMemberViewModel.h"
#include "Foundation/UserData/CXUserDataSubsystem.h"
#include "Tags/CXGameplayTags.h"

// TODO : CXLOG Category 정리 후 제거
#include "UI/CXUIManagerSubsystem.h"

#include <Engine/GameInstance.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXPartyViewModel)

FMVVMViewModelContext UCXPartyViewModel::MakeGlobalContext()
{
	FMVVMViewModelContext Context;
	Context.ContextClass = StaticClass();
	Context.ContextName = TEXT("PartyViewModel");
	return Context;
}

void UCXPartyViewModel::Initialize(UGameInstance& InGameInstance)
{
	Reset();

	UserDataSubsystem = InGameInstance.GetSubsystem<UCXUserDataSubsystem>();
	if (false == UserDataSubsystem.IsValid())
	{
		UE_LOG(LogCXUI, Error, TEXT("PartyViewModel : UserDataSubsystem이 없어 파티 정보를 표시하지 않습니다."));
		return;
	}

	if (UCXGameplayMessageSubsystem* Messages = InGameInstance.GetSubsystem<UCXGameplayMessageSubsystem>())
	{
		PartyChangedListener = Messages->RegisterListener(CXVIGameplayTags::Message_UserData_PartyChanged, this, &ThisClass::HandlePartyChanged);
	}

	RebuildMembers();
}

void UCXPartyViewModel::Reset()
{
	PartyChangedListener.Unregister();
	UserDataSubsystem.Reset();

	Members.Reset();
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Members);
}

void UCXPartyViewModel::HandlePartyChanged(const FGameplayTag& InChannel, const FCXPartyChangedMessage& InMessage)
{
	const UCXUserDataSubsystem* UserData = UserDataSubsystem.Get();
	if (nullptr == UserData)
	{
		return;
	}

	// 파티 전체 변경(새 게임 / 불러오기) 또는 인원 변경 → 목록 재구성
	if (INDEX_NONE == InMessage.MemberIndex || Members.Num() != UserData->GetPartyMembers().Num())
	{
		RebuildMembers();
		return;
	}

	RefreshMember(InMessage.MemberIndex);
}

void UCXPartyViewModel::RebuildMembers()
{
	Members.Reset();

	if (const UCXUserDataSubsystem* UserData = UserDataSubsystem.Get())
	{
		const int32 MemberNum = UserData->GetPartyMembers().Num();
		for (int32 Index = 0; Index < MemberNum; ++Index)
		{
			Members.Emplace(NewObject<UCXPartyMemberViewModel>(this));
			RefreshMember(Index);
		}
	}

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Members);
}

void UCXPartyViewModel::RefreshMember(int32 InMemberIndex)
{
	const UCXUserDataSubsystem* UserData = UserDataSubsystem.Get();
	if (nullptr == UserData || false == Members.IsValidIndex(InMemberIndex))
	{
		return;
	}

	const TArray<FCXPartyMemberState>& Party = UserData->GetPartyMembers();
	if (false == Party.IsValidIndex(InMemberIndex))
	{
		return;
	}

	const FCXPartyMemberState& Member = Party[InMemberIndex];
	Members[InMemberIndex]->Refresh(Member, UserData->GetCharacterDefinition(Member.CharacterId), UserData->GetMemberMaxHealth(InMemberIndex));
}

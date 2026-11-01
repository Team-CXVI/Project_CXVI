// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/CXPotionViewModel.h"

#include "Foundation/GameplayData/CXGameplayDataSettings.h"
#include "Foundation/GameplayData/CXGameplayDataSubsystem.h"
#include "Foundation/UserData/CXUserDataSubsystem.h"
#include "Tags/CXGameplayTags.h"

// TODO : CXLOG Category 정리 후 제거
#include "UI/CXUIManagerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXPotionViewModel)

namespace CXPotionViewModel
{
	// 설정의 포션 ItemId (지정되지 않으면 None)
	static FName GetPotionItemId()
	{
		return GetDefault<UCXGameplayDataSettings>()->PotionItemId;
	}
}

FMVVMViewModelContext UCXPotionViewModel::MakeGlobalContext()
{
	FMVVMViewModelContext Context;
	Context.ContextClass = StaticClass();
	Context.ContextName = TEXT("PotionViewModel");
	return Context;
}

void UCXPotionViewModel::Initialize(UGameInstance& InGameInstance)
{
	Reset();

	UserDataSubsystem = InGameInstance.GetSubsystem<UCXUserDataSubsystem>();
	if (false == UserDataSubsystem.IsValid())
	{
		UE_LOG(LogCXUI, Error, TEXT("PotionViewModel : UserDataSubsystem이 없어 포션 정보를 표시하지 않습니다."));
		return;
	}

	if (CXPotionViewModel::GetPotionItemId().IsNone())
	{
		UE_LOG(LogCXUI, Warning, TEXT("PotionViewModel : PotionItemId가 지정되지 않았습니다. (Project Settings > Game > CXVI Gameplay Data > Item)"));
	}

	if (UCXGameplayMessageSubsystem* Messages = InGameInstance.GetSubsystem<UCXGameplayMessageSubsystem>())
	{
		InventoryChangedListener = Messages->RegisterListener(CXVIGameplayTags::Message_UserData_InventoryChanged, this, &ThisClass::HandleInventoryChanged);
	}

	RefreshIcon(InGameInstance);
	RefreshCount();
}

void UCXPotionViewModel::Reset()
{
	InventoryChangedListener.Unregister();
	UserDataSubsystem.Reset();

	SetIcon(TSoftObjectPtr<UTexture2D>());
	SetCount(0);
}

FText UCXPotionViewModel::GetCountText() const
{
	return FText::AsNumber(Count);
}

bool UCXPotionViewModel::IsUsable() const
{
	return Count > 0;
}

void UCXPotionViewModel::HandleInventoryChanged(const FGameplayTag& InChannel, const FCXInventoryChangedMessage& InMessage)
{
	// 인벤토리 전체 변경(새 게임 / 불러오기) → 다시 읽음
	if (InMessage.ItemId.IsNone())
	{
		RefreshCount();
		return;
	}

	if (InMessage.ItemId == CXPotionViewModel::GetPotionItemId())
	{
		SetCount(InMessage.NewCount);
	}
}

void UCXPotionViewModel::RefreshIcon(UGameInstance& InGameInstance)
{
	const UCXGameplayDataSubsystem* GameplayData = InGameInstance.GetSubsystem<UCXGameplayDataSubsystem>();
	const FCXItemRow* ItemRow = GameplayData ? GameplayData->FindItemRow(CXPotionViewModel::GetPotionItemId()) : nullptr;
	SetIcon(ItemRow ? ItemRow->Icon : TSoftObjectPtr<UTexture2D>());
}

void UCXPotionViewModel::RefreshCount()
{
	const UCXUserDataSubsystem* UserData = UserDataSubsystem.Get();
	const FName PotionItemId = CXPotionViewModel::GetPotionItemId();
	SetCount((UserData && false == PotionItemId.IsNone()) ? UserData->GetItemCount(PotionItemId) : 0);
}

void UCXPotionViewModel::SetIcon(const TSoftObjectPtr<UTexture2D>& InValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(Icon, InValue);
}

void UCXPotionViewModel::SetCount(int32 InValue)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(Count, InValue))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCountText);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsUsable);
	}
}

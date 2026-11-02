// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXSaveSlotEntryWidget.h"

#include "UI/CXUIManagerSubsystem.h"
#include "Foundation/UserData/CXUserDataSubsystem.h"
#include "Foundation/UserData/CXCharacterDefinition.h"
#include "Level/CXLevelDefinition.h"
#include "Level/CXLevelTransitionSubsystem.h"
#include "CXSaveSlotMemberWidget.h"

#include <CommonTextBlock.h>
#include <Components/Image.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXSaveSlotEntryWidget)

void UCXSaveSlotEntryWidget::InitSlot(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;

	// 베이스의 NativePreConstruct가 ButtonText를 다시 적용하므로 함께 설정
	ButtonText = (UCXUserDataSubsystem::AutoSaveSlotIndex == InSlotIndex)
		? AutoSlotName
		: FText::FormatNamed(ManualSlotNameFormat, TEXT("Index"), FText::AsNumber(InSlotIndex));

	if (ButtonTextBlock)
	{
		ButtonTextBlock->SetText(ButtonText);
	}
}

bool UCXSaveSlotEntryWidget::RefreshSlot()
{
	const UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this);

	FCXSaveHeader Header;
	TArray<FCXPartyMemberState> Party;
	if (nullptr == UserData || false == UserData->GetSlotInfo(SlotIndex, Header, Party))
	{
		ShowEmptySlot();
		return false;
	}

	const int32 TotalSeconds = FMath::FloorToInt32(Header.PlayTimeSeconds);

	SaveTimeText->SetText(FText::FromString(Header.SaveTime.ToString(TEXT("%Y-%m-%d %H:%M"))));
	PlayTimeText->SetText(FText::FromString(FString::Printf(TEXT("%d:%02d:%02d"), TotalSeconds / 3600, (TotalSeconds / 60) % 60, TotalSeconds % 60)));

	RefreshLevel(Header);
	RefreshParty(Party);
	return true;
}

void UCXSaveSlotEntryWidget::ShowEmptySlot()
{
	SaveTimeText->SetText(EmptySlotText);
	PlayTimeText->SetText(FText::GetEmpty());
	LevelNameText->SetText(FText::GetEmpty());
	LevelImage->SetVisibility(ESlateVisibility::Hidden);
	PartyMemberBox->ClearChildren();
}

void UCXSaveSlotEntryWidget::RefreshLevel(const FCXSaveHeader& InHeader)
{
	// TODO : 임시. 레벨 표시 이름의 위치가 정해지면 교체 (지금은 레벨 정의의 에셋 이름)
	LevelNameText->SetText(FText::FromName(InHeader.LevelId.PrimaryAssetName));

	const UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this);
	const UCXLevelDefinition* LevelDefinition = LevelTransition ? LevelTransition->LoadLevelDefinition(InHeader.LevelId) : nullptr;
	if (nullptr == LevelDefinition || LevelDefinition->PreviewImage.IsNull())
	{
		LevelImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	LevelImage->SetBrushFromSoftTexture(LevelDefinition->PreviewImage);
	LevelImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UCXSaveSlotEntryWidget::RefreshParty(const TArray<FCXPartyMemberState>& InParty)
{
	PartyMemberBox->ClearChildren();

	if (nullptr == MemberClass)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] MemberClass가 지정되지 않았습니다."), *GetName());
		return;
	}

	const UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this);
	if (nullptr == UserData)
	{
		return;
	}

	for (const FCXPartyMemberState& Member : InParty)
	{
		const UCXCharacterDefinition* Definition = UserData->LoadCharacterDefinition(Member.CharacterId);

		UCXSaveSlotMemberWidget* MemberWidget = CreateWidget<UCXSaveSlotMemberWidget>(this, MemberClass);
		MemberWidget->SetMember(Definition ? Definition->DisplayName : FText::GetEmpty(), Member.Level);

		PartyMemberBox->AddChild(MemberWidget);
	}
}

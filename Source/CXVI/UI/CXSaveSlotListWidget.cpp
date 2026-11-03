// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXSaveSlotListWidget.h"

#include "UI/CXUIManagerSubsystem.h"
#include "UI/CXSaveSlotEntryWidget.h"
#include "Foundation/UserData/CXUserDataSubsystem.h"
#include "Tags/CXGameplayTags.h"

#include <Components/PanelWidget.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXSaveSlotListWidget)

UCXSaveSlotListWidget::UCXSaveSlotListWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InputMode = ECXWidgetInputMode::Menu;

	// 뒤로 가기 입력처리를 활성화
	bIsBackHandler = true;
}

void UCXSaveSlotListWidget::SetScreenContext(const FGameplayTag& InContextTag)
{
	ScreenContext = InContextTag;

	// 이미 맨 위에 열려 있으면 NativeOnActivated가 다시 호출되지 않으므로 여기서 갱신
	if (IsActivated())
	{
		RefreshEntries();
	}
}

void UCXSaveSlotListWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (nullptr == SlotEntryClass)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] SlotEntryClass가 지정되지 않았습니다."), *GetName());
		return;
	}

	const UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this);
	if (nullptr == UserData)
	{
		return;
	}

	for (const int32 SlotIndex : UserData->GetAllSlotIndices())
	{
		UCXSaveSlotEntryWidget* Entry = CreateWidget<UCXSaveSlotEntryWidget>(this, SlotEntryClass);
		Entry->InitSlot(SlotIndex);
		Entry->OnClicked().AddUObject(this, &ThisClass::HandleSlotClicked, SlotIndex);

		SlotEntryBox->AddChild(Entry);
		SlotEntries.Add(Entry);
	}
}

void UCXSaveSlotListWidget::NativeOnActivated()
{	
	SaveFinishedListener = UCXGameplayMessageSubsystem::Get(this).RegisterListener(CXVIGameplayTags::Message_UserData_SaveFinished, this, &ThisClass::HandleSaveFinished);

	// 포커스 대상이 활성화 여부로 정해지므로 Super(포커스 대상 확인)보다 먼저 갱신
	RefreshEntries();

	Super::NativeOnActivated();
}

void UCXSaveSlotListWidget::NativeOnDeactivated()
{
	SaveFinishedListener.Unregister();

	Super::NativeOnDeactivated();
}

UWidget* UCXSaveSlotListWidget::NativeGetDesiredFocusTarget() const
{
	for (UCXSaveSlotEntryWidget* Entry : SlotEntries)
	{
		if (Entry->GetIsEnabled())
		{
			return Entry;
		}
	}

	return Super::NativeGetDesiredFocusTarget();
}

void UCXSaveSlotListWidget::RefreshEntries()
{
	const bool bSaveMode = (ScreenContext == CXVIGameplayTags::UI_ScreenContext_SaveSlot_Save);

	for (UCXSaveSlotEntryWidget* Entry : SlotEntries)
	{
		const bool bHasSaveFile = Entry->RefreshSlot();

		// 저장하기 : 자동 저장 슬롯만 제외 / 불러오기 : 저장 파일이 있는 슬롯만
		Entry->SetIsEnabled(bSaveMode ? (UCXUserDataSubsystem::AutoSaveSlotIndex != Entry->GetSlotIndex()) : bHasSaveFile);
	}
}

void UCXSaveSlotListWidget::HandleSlotClicked(int32 InSlotIndex)
{
	if (ScreenContext == CXVIGameplayTags::UI_ScreenContext_SaveSlot_Load)
	{
		HandleLoadSlotClicked(InSlotIndex);
	}
	else if (ScreenContext == CXVIGameplayTags::UI_ScreenContext_SaveSlot_Save)
	{
		HandleSaveSlotClicked(InSlotIndex);
	}
	else
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] 지원하지 않는 ScreenContext입니다: %s"), *GetName(), *ScreenContext.ToString());
	}
}

void UCXSaveSlotListWidget::ConfirmLoad(int32 InSlotIndex)
{
	// 페이드 동안 화면이 타이틀로 돌아가지 않도록 닫지 않음. 레벨이 바뀌면 함께 제거됨
	if (UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this))
	{
		UserData->LoadGameAndTravel(InSlotIndex);
	}
}

void UCXSaveSlotListWidget::ConfirmSave(int32 InSlotIndex)
{
	// 저장 위치는 깃발 메뉴가 열릴 때 기록됨. 표시 갱신은 Message.UserData.SaveFinished에서 처리
	UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this);
	if (nullptr == UserData || false == UserData->SaveGame(InSlotIndex))
	{
		ShowSaveFailedPopup();
	}
}

void UCXSaveSlotListWidget::HandleLoadSlotClicked(int32 InSlotIndex)
{
	if (UCXUIManagerSubsystem* UIManager = UCXUIManagerSubsystem::TryGet(this))
	{
		// TODO : 텍스트 데이터 테이블 작업 때 본문 ID 지정
		UIManager->ShowConfirmCancelPopup(NAME_None, FSimpleDelegate::CreateUObject(this, &ThisClass::ConfirmLoad, InSlotIndex));
	}
}

void UCXSaveSlotListWidget::HandleSaveSlotClicked(int32 InSlotIndex)
{
	const UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this);
	if (nullptr == UserData)
	{
		return;
	}

	// 빈 슬롯은 잃을 데이터가 없으므로 바로 저장
	if (false == UserData->DoesSlotExist(InSlotIndex))
	{
		ConfirmSave(InSlotIndex);
		return;
	}

	if (UCXUIManagerSubsystem* UIManager = UCXUIManagerSubsystem::TryGet(this))
	{
		// TODO : 텍스트 데이터 테이블 작업 때 본문 ID 지정
		UIManager->ShowConfirmCancelPopup(NAME_None, FSimpleDelegate::CreateUObject(this, &ThisClass::ConfirmSave, InSlotIndex));
	}
}

void UCXSaveSlotListWidget::HandleSaveFinished(const FGameplayTag& InChannel, const FCXSaveFinishedMessage& InMessage)
{
	RefreshEntries();

	// 자동 저장은 이 화면에서 요청한 저장이 아니므로 알리지 않음
	if (false == InMessage.bSuccess && UCXUserDataSubsystem::AutoSaveSlotIndex != InMessage.SlotIndex)
	{
		ShowSaveFailedPopup();
	}
}

void UCXSaveSlotListWidget::ShowSaveFailedPopup()
{
	if (UCXUIManagerSubsystem* UIManager = UCXUIManagerSubsystem::TryGet(this))
	{
		// TODO : 텍스트 데이터 테이블 작업 때 본문 ID 지정
		UIManager->ShowConfirmPopup(NAME_None);
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXSaveSlotListWidget.h"

#include "UI/CXUIManagerSubsystem.h"
#include "UI/CXSaveSlotEntryWidget.h"
#include "Foundation/UserData/CXUserDataSubsystem.h"

#include <Components/PanelWidget.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXSaveSlotListWidget)

UCXSaveSlotListWidget::UCXSaveSlotListWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InputMode = ECXWidgetInputMode::Menu;

	// 뒤로 가기 입력처리를 활성화
	bIsBackHandler = true;
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
	// 포커스 대상이 활성화 여부로 정해지므로 Super(포커스 대상 확인)보다 먼저 갱신
	for (UCXSaveSlotEntryWidget* Entry : SlotEntries)
	{
		Entry->SetIsEnabled(Entry->RefreshSlot());
	}

	Super::NativeOnActivated();
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

void UCXSaveSlotListWidget::HandleSlotClicked(int32 InSlotIndex)
{
	// 페이드 동안 화면이 타이틀로 돌아가지 않도록 닫지 않음. 레벨이 바뀌면 함께 제거됨
	if (UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this))
	{
		UserData->LoadGameAndTravel(InSlotIndex);
	}
}

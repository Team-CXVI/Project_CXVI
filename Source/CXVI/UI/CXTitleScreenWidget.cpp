// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXTitleScreenWidget.h"
#include "CXButtonBase.h"
#include "Foundation/UserData/CXUserDataSubsystem.h"
#include "Level/CXLevelTransitionSubsystem.h"
#include "Level/CXLevelTransitionSettings.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"
#include "UI/CXUITypes.h"
#include "Tags/CXGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXTitleScreenWidget)

UCXTitleScreenWidget::UCXTitleScreenWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	// 타이틀에는 UI 밖에 없으므로 게임 입력 처리는 하지 않음
	InputMode = ECXWidgetInputMode::Menu;
}

void UCXTitleScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	StartButton->OnClicked().AddUObject(this, &ThisClass::HandleStartClicked);
	ContinueButton->OnClicked().AddUObject(this, &ThisClass::HandleContinueClicked);
	QuitButton->OnClicked().AddUObject(this, &ThisClass::HandleQuitClicked);
}

void UCXTitleScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	const UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this);
	ContinueButton->SetIsEnabled(UserData && UserData->HasAnySaveSlot());
}

void UCXTitleScreenWidget::HandleStartClicked()
{
	UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this);
	if (nullptr == LevelTransition || LevelTransition->IsTransitioning())
	{
		return;
	}

	// 이전 게임(타이틀 복귀 전) 데이터를 새 게임 데이터로 초기화
	if (UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this))
	{
		UserData->StartNewGame();
	}

	LevelTransition->TravelToLevel(GetDefault<UCXLevelTransitionSettings>()->NewGameLevel, NAME_None);
}

void UCXTitleScreenWidget::HandleContinueClicked()
{
	const UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this);
	if (nullptr == LevelTransition || LevelTransition->IsTransitioning())
	{
		return;
	}

	FCXOpenScreenMessage Message;
	Message.ScreenTag = CXVIGameplayTags::UI_Screen_SaveSlot;
	Message.ContextTag = CXVIGameplayTags::UI_ScreenContext_SaveSlot_Load;
	UCXGameplayMessageSubsystem::Get(this).BroadcastMessage(CXVIGameplayTags::Message_UI_OpenScreen, Message);
}

void UCXTitleScreenWidget::HandleQuitClicked()
{
	if (UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this))
	{
		LevelTransition->QuitGame();
	}
}
// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Field/CXFieldMenuWidget.h"

#include "UI/CXButtonBase.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"
#include "Foundation/UserData/CXUserDataSubsystem.h"
#include "Level/CXLevelTransitionSubsystem.h"
#include "UI/CXUIManagerSubsystem.h"
#include "Tags/CXGameplayTags.h"
#include "UI/CXUITypes.h"

#include <Kismet/GameplayStatics.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldMenuWidget)

UCXFieldMenuWidget::UCXFieldMenuWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InputMode = ECXWidgetInputMode::Menu;

	// 뒤로 가기 입력처리를 활성화
	bIsBackHandler = true;
}

void UCXFieldMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ContinueButton->OnClicked().AddUObject(this, &ThisClass::HandleContinueClicked);
	LoadGameButton->OnClicked().AddUObject(this, &ThisClass::HandleLoadGameClicked);
	TitleButton->OnClicked().AddUObject(this, &ThisClass::HandleTitleClicked);
	QuitButton->OnClicked().AddUObject(this, &ThisClass::HandleQuitClicked);
}

void UCXFieldMenuWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	const UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this);
	LoadGameButton->SetIsEnabled(UserData && UserData->HasAnySaveSlot());

	// 인 게임은 일시 정지
	UGameplayStatics::SetGamePaused(this, true);
}

void UCXFieldMenuWidget::NativeOnDeactivated()
{
	UGameplayStatics::SetGamePaused(this, false);

	Super::NativeOnDeactivated();
}

void UCXFieldMenuWidget::HandleContinueClicked()
{
	DeactivateWidget();
}

void UCXFieldMenuWidget::HandleLoadGameClicked()
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

void UCXFieldMenuWidget::HandleTitleClicked()
{
	if (UCXUIManagerSubsystem* UIManager = UCXUIManagerSubsystem::TryGet(this))
	{
		// TODO : 텍스트 데이터 테이블 작업 때 본문 ID 지정
		UIManager->ShowConfirmCancelPopup(NAME_None, FSimpleDelegate::CreateUObject(this, &ThisClass::ConfirmReturnToTitle));
	}
}

void UCXFieldMenuWidget::HandleQuitClicked()
{
	if (UCXUIManagerSubsystem* UIManager = UCXUIManagerSubsystem::TryGet(this))
	{
		// TODO : 텍스트 데이터 테이블 작업 때 본문 ID 지정
		UIManager->ShowConfirmCancelPopup(NAME_None, FSimpleDelegate::CreateUObject(this, &ThisClass::ConfirmQuit));
	}
}

void UCXFieldMenuWidget::ConfirmReturnToTitle()
{
	UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this);
	if (nullptr == LevelTransition || LevelTransition->IsTransitioning())
	{
		return;
	}

	// 정지 상태로 이동하지 않도록 먼저 닫아서 정지 해제
	DeactivateWidget();

	LevelTransition->TravelToTitle();
}

void UCXFieldMenuWidget::ConfirmQuit()
{
	if (UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this))
	{
		LevelTransition->QuitGame();
	}
}

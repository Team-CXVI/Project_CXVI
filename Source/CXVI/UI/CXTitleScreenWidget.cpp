// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXTitleScreenWidget.h"
#include "CXButtonBase.h"
#include "Level/CXLevelTransitionSubsystem.h"
#include "Level/CXLevelTransitionSettings.h"


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
	QuitButton->OnClicked().AddUObject(this, &ThisClass::HandleQuitClicked);
}

void UCXTitleScreenWidget::HandleStartClicked()
{
	// 레벨 전환 중 중복 요청은 LevelTransition쪽에서 거부 처리
	if (UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this))
	{
		LevelTransition->TravelToLevel(GetDefault<UCXLevelTransitionSettings>()->NewGameLevel, NAME_None);
	}
}

void UCXTitleScreenWidget::HandleQuitClicked()
{
	if (UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this))
	{
		LevelTransition->QuitGame();
	}
}

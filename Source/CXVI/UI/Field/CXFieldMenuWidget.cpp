// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Field/CXFieldMenuWidget.h"

#include "UI/CXButtonBase.h"
#include "Level/CXLevelTransitionSubsystem.h"

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
	TitleButton->OnClicked().AddUObject(this, &ThisClass::HandleTitleClicked);
	QuitButton->OnClicked().AddUObject(this, &ThisClass::HandleQuitClicked);
}

void UCXFieldMenuWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

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

void UCXFieldMenuWidget::HandleTitleClicked()
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

void UCXFieldMenuWidget::HandleQuitClicked()
{
	if (UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this))
	{
		LevelTransition->QuitGame();
	}
}

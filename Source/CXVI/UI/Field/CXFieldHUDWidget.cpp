// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Field/CXFieldHUDWidget.h"

#include "CXFieldMenuWidget.h"
#include "UI/CXUIManagerSubsystem.h"
#include "Tags/CXGameplayTags.h"

#include <Input/CommonUIInputTypes.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldHUDWidget)

UCXFieldHUDWidget::UCXFieldHUDWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InputMode = ECXWidgetInputMode::Game;
}

void UCXFieldHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (nullptr == OpenMenuAction)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] OpenMenuAction이 지정되지 않았습니다."), *GetName());
		return;
	}

	// 메뉴 열기 InputAction은 UI에서 바인딩 -> CommonUI에서 처리 
	// 입력은 InputMode가 Game 모드에서 받도록 지정
	FBindUIActionArgs BindArgs(OpenMenuAction, /*bShouldDisplayInActionBar*/ false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleOpenMenu));
	BindArgs.InputMode = ECommonInputMode::Game;

	RegisterUIActionBinding(BindArgs);
}

void UCXFieldHUDWidget::HandleOpenMenu()
{
	if (nullptr == MenuClass)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] MenuClass가 지정되지 않았습니다."), *GetName());
		return;
	}

	UCXUIManagerSubsystem* UIManager = UCXUIManagerSubsystem::TryGet(this);
	if (nullptr == UIManager)
	{
		return;
	}

	UIManager->PushWidgetToLayer<UCXFieldMenuWidget>(CXVIGameplayTags::UI_Layer_GameMenu, MenuClass);
}

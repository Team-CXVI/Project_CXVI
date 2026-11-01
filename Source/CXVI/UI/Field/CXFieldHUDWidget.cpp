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

void UCXFieldHUDWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	const UCXUIManagerSubsystem* UIManager = UCXUIManagerSubsystem::TryGet(this);
	const UCXPrimaryGameLayout* PrimaryLayout = UIManager ? UIManager->GetPrimaryLayout() : nullptr;
	UCommonActivatableWidgetContainerBase* Layer = PrimaryLayout ? PrimaryLayout->GetLayerWidget(CXVIGameplayTags::UI_Layer_GameMenu) : nullptr;
	if (nullptr == Layer)
	{
		return;
	}

	GameMenuLayer = Layer;
	GameMenuLayerChangedHandle = Layer->OnDisplayedWidgetChanged().AddUObject(this, &ThisClass::HandleGameMenuLayerChanged);
}

void UCXFieldHUDWidget::NativeOnDeactivated()
{
	if (UCommonActivatableWidgetContainerBase* Layer = GameMenuLayer.Get())
	{
		Layer->OnDisplayedWidgetChanged().Remove(GameMenuLayerChangedHandle);
	}

	GameMenuLayer.Reset();
	GameMenuLayerChangedHandle.Reset();

	// 풀에서 재사용될 때 숨겨진 채로 나오지 않도록 복원
	SetContentHidden(false);

	Super::NativeOnDeactivated();
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

void UCXFieldHUDWidget::HandleGameMenuLayerChanged(UCommonActivatableWidget* InDisplayedWidget)
{
	SetContentHidden(nullptr != InDisplayedWidget);
}

void UCXFieldHUDWidget::SetContentHidden(bool bInHidden)
{
	// 이미 숨긴 상태에서 다시 저장하면 Collapsed가 원래 값으로 저장되므로 상태가 바뀔 때만 처리
	if (bContentHidden == bInHidden)
	{
		return;
	}

	UWidget* RootWidget = GetRootWidget();
	if (nullptr == RootWidget)
	{
		return;
	}

	bContentHidden = bInHidden;

	if (bInHidden)
	{
		CachedRootVisibility = RootWidget->GetVisibility();
		RootWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		RootWidget->SetVisibility(CachedRootVisibility);
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXCombatHUDWidget.h"

#include "UI/ViewModel/CXCombatViewModel.h"

#include <MVVMSubsystem.h>
#include <View/MVVMView.h>

// TODO : CXLOG Category 정리 후 대체
#include "UI/CXUIManagerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatHUDWidget)


UCXCombatHUDWidget::UCXCombatHUDWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InputMode = ECXWidgetInputMode::GameAndMenu;
}

void UCXCombatHUDWidget::SetCombatViewModel(UCXCombatViewModel* InViewModel)
{
	UMVVMView* View = UMVVMSubsystem::GetViewFromUserWidget(this);
	if (nullptr == View || false == View->SetViewModelByClass(InViewModel))
	{
		UE_LOG(LogCXUI, Error, TEXT("%s : CombatViewModel을 연결하지 못했습니다. WBP Viewmodels에 CXCombatViewModel(Manual)이 있는지, View Binding이 1개 이상 있는지 확인하세요."), *GetName());
	}
}

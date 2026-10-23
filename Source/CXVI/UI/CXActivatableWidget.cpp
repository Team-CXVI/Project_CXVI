// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CXActivatableWidget.h"
#include "UI/CXUIManagerSubsystem.h"

#if WITH_EDITOR
#include "Blueprint/WidgetTree.h"
#include "Editor/WidgetCompilerLog.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXActivatableWidget)

TOptional<FUIInputConfig> UCXActivatableWidget::GetDesiredInputConfig() const
{
	switch (InputMode)
	{
	case ECXWidgetInputMode::GameAndMenu:
		return FUIInputConfig(ECommonInputMode::All, GameMouseCaptureMode);

	case ECXWidgetInputMode::Game:
		return FUIInputConfig(ECommonInputMode::Game, GameMouseCaptureMode);

	case ECXWidgetInputMode::Menu:
		return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);

	case ECXWidgetInputMode::Default:
	default:
		return TOptional<FUIInputConfig>();
	}
}

UWidget* UCXActivatableWidget::NativeGetDesiredFocusTarget() const
{
	return nullptr;
}

void UCXActivatableWidget::NativeOnActivated()
{
	Super::NativeOnActivated();


}

#if WITH_EDITOR
void UCXActivatableWidget::ValidateCompiledWidgetTree(const UWidgetTree& BlueprintWidgetTree, IWidgetCompilerLog& CompileLog) const
{
	Super::ValidateCompiledWidgetTree(BlueprintWidgetTree, CompileLog);


}
#endif // WITH_EDITOR
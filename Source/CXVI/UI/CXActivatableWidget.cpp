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
	// BP쪽의 Get Desired Focus Target 우선
	if (UWidget* Target = Super::NativeGetDesiredFocusTarget())
	{
		return Target;
	}

	return DefaultFocusWidgetName.IsNone() ? nullptr : GetWidgetFromName(DefaultFocusWidgetName);
}

void UCXActivatableWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

#if !UE_BUILD_SHIPPING
	if (InputMode == ECXWidgetInputMode::Menu && nullptr == GetDesiredFocusTarget())
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] Menu 화면인데 포커스 대상이 없습니다. 게임패드로 조작할 수 없습니다."), *GetName());
	}
#endif
}

#if WITH_EDITOR
void UCXActivatableWidget::ValidateCompiledWidgetTree(const UWidgetTree& BlueprintWidgetTree, IWidgetCompilerLog& CompileLog) const
{
	Super::ValidateCompiledWidgetTree(BlueprintWidgetTree, CompileLog);

	if (InputMode != ECXWidgetInputMode::Menu)
	{
		return;
	}

	const bool bHasBPFocusTarget = GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(UCXActivatableWidget, BP_GetDesiredFocusTarget));
	if (bHasBPFocusTarget)
	{
		return;
	}

	if (DefaultFocusWidgetName.IsNone())
	{
		CompileLog.Warning(FText::FromString(
			TEXT("Input Mode가 Menu인데 포커스 대상이 없습니다. Default Focus Widget Name을 지정하거나 Get Desired Focus Target을 구현하세요.")));
	}
	else if (nullptr == BlueprintWidgetTree.FindWidget(DefaultFocusWidgetName))
	{
		CompileLog.Error(FText::FromString(FString::Printf(
			TEXT("Default Focus Widget Name '%s'에 해당하는 위젯이 없습니다."), *DefaultFocusWidgetName.ToString())));
	}

}
#endif // WITH_EDITOR
// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXInputIconWidget.h"

#include "CXUIManagerSubsystem.h"

#include <Components/Image.h>
#include <CommonInputSubsystem.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXInputIconWidget)

void UCXInputIconWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UCommonInputSubsystem* InputSubsystem = UCommonInputSubsystem::Get(GetOwningLocalPlayer()))
	{
		InputSubsystem->OnInputMethodChangedNative.AddUObject(this, &ThisClass::HandleInputMethodChanged);
		RefreshIcon(InputSubsystem->GetCurrentInputType());
	}
}

void UCXInputIconWidget::NativeDestruct()
{
	if (UCommonInputSubsystem* InputSubsystem = UCommonInputSubsystem::Get(GetOwningLocalPlayer()))
	{
		InputSubsystem->OnInputMethodChangedNative.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UCXInputIconWidget::HandleInputMethodChanged(ECommonInputType InInputType)
{
	RefreshIcon(InInputType);
}

void UCXInputIconWidget::RefreshIcon(ECommonInputType InInputType)
{
	const UCommonInputPlatformSettings* PlatformSettings = UCommonInputPlatformSettings::Get();
	if (nullptr == PlatformSettings)
	{
		return;
	}

	const UCommonInputSubsystem* InputSubsystem = UCommonInputSubsystem::Get(GetOwningLocalPlayer());
	if(nullptr == InputSubsystem)
	{
		return;
	}

	const FKey& Key = (InInputType == ECommonInputType::Gamepad) ? GamepadKey : KeyboardKey;
	const FName GamepadName = InputSubsystem->GetCurrentGamepadName();

	// 입력 키 아이콘 브러쉬
	FSlateBrush Brush;
	if (PlatformSettings->TryGetInputBrush(Brush, Key, InInputType, GamepadName))
	{
		InputIcon->SetBrush(Brush);
	}
	else
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] '%s' 키의 아이콘이 Controller Data에 없습니다."), *GetName(), *Key.ToString());
	}
}
  
// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXInputIconWidget.h"

#include "CXUIManagerSubsystem.h"

#include <Components/Image.h>
#include <CommonInputSubsystem.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXInputIconWidget)

void UCXInputIconWidget::SetKeys(const FKey& InKeyboardKey, const FKey& InGamepadKey)
{
	KeyboardKey = InKeyboardKey;
	GamepadKey = InGamepadKey;

	// 표시 전이면 NativeConstruct에서 갱신됨
	if (GetCachedWidget().IsValid())
	{
		if (const UCommonInputSubsystem* InputSubsystem = UCommonInputSubsystem::Get(GetOwningLocalPlayer()))
		{
			RefreshIcon(InputSubsystem->GetCurrentInputType());
		}
	}
}

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
	if (nullptr == InputSubsystem)
	{
		return;
	}

	const FKey& Key = (InInputType == ECommonInputType::Gamepad) ? GamepadKey : KeyboardKey;
	const FName GamepadName = InputSubsystem->GetCurrentGamepadName();

	// 현재 입력 장치에 지정된 키가 없으면 표시하지 않음
	if (false == Key.IsValid())
	{
		InputIcon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	InputIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

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
  
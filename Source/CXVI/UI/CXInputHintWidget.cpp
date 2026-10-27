// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXInputHintWidget.h"
#include "CXUIManagerSubsystem.h"

#include <Components/TextBlock.h>
#include <Components/Image.h>
#include <CommonInputSubsystem.h>
#include <Blueprint/WidgetTree.h>
#include <Components/HorizontalBox.h>
#include <Components/HorizontalBoxSlot.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXInputHintWidget)

void UCXInputHintWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 여기서 초기화 해야 디자이너 창에서도 반영됨
	if (LabelText)
	{
		LabelText->SetText(Label);
	}
}

void UCXInputHintWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UCommonInputSubsystem* InputSubsystem = UCommonInputSubsystem::Get(GetOwningLocalPlayer()))
	{
		InputSubsystem->OnInputMethodChangedNative.AddUObject(this, &ThisClass::HandleInputMethodChanged);
		RefreshIcons(InputSubsystem->GetCurrentInputType());
	}
}

void UCXInputHintWidget::NativeDestruct()
{
	if (UCommonInputSubsystem* InputSubsystem = UCommonInputSubsystem::Get(GetOwningLocalPlayer()))
	{
		InputSubsystem->OnInputMethodChangedNative.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UCXInputHintWidget::HandleInputMethodChanged(ECommonInputType InInputType)
{
	RefreshIcons(InInputType);
}

void UCXInputHintWidget::RefreshIcons(ECommonInputType InInputType)
{
	IconBox->ClearChildren();

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

	const TArray<FKey>& Keys = (InInputType == ECommonInputType::Gamepad) ? GamepadKeys : KeyboardKeys;
	const FName GamepadName = InputSubsystem->GetCurrentGamepadName();

	// Key Set 우선 처리
	FSlateBrush Brush;
	if (PlatformSettings->TryGetInputBrush(Brush, Keys, InInputType, GamepadName))
	{
		AddIcon(Brush);
		return;
	}

	// 각 개별 Key 대응
	for (const FKey& Key : Keys)
	{
		if (PlatformSettings->TryGetInputBrush(Brush, Key, InInputType, GamepadName))
		{
			AddIcon(Brush);
		}
		else
		{
			UE_LOG(LogCXUI, Warning, TEXT("[%s] '%s' 키의 아이콘이 Controller Data에 없습니다."), *GetName(), *Key.ToString());
		}
	}
}

void UCXInputHintWidget::AddIcon(const FSlateBrush& InBrush)
{
	UImage* Icon = WidgetTree->ConstructWidget<UImage>();
	Icon->SetBrush(InBrush);

	if (UHorizontalBoxSlot* IconSlot = IconBox->AddChildToHorizontalBox(Icon))
	{
		IconSlot->SetPadding(IconPadding);
		IconSlot->SetVerticalAlignment(VAlign_Center);
	}
}

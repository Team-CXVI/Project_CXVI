// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXButtonBase.h"

#include <CommonTextBlock.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXButtonBase)

UCXButtonBase::UCXButtonBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 포커스 하이라이트용 Selected 설정
	bSelectable = true;
	bShouldSelectUponReceivingFocus = true;

	// 하이라이트(Selected) 상태에서도 클릭되어야 함
	bInteractableWhenSelected = true;
	bToggleable = false;
}

void UCXButtonBase::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 디자이너 편집창에서도 글자가 보이도록 PreConstruct에서 설정
	if (ButtonTextBlock)
	{
		ButtonTextBlock->SetText(ButtonText);
	}
}

void UCXButtonBase::HandleFocusLost()
{
	Super::HandleFocusLost();

	// bToggleable이 꺼져 있으면 SetIsSelected(false)로는 해제되지 않으므로 ClearSelection 사용
	ClearSelection();
}

void UCXButtonBase::NativeOnHovered()
{
	Super::NativeOnHovered();

	if (IsInteractionEnabled())
	{
		SetFocus();
	}
}

void UCXButtonBase::NativeOnCurrentTextStyleChanged()
{
	Super::NativeOnCurrentTextStyleChanged();

	if (ButtonTextBlock)
	{
		ButtonTextBlock->SetStyle(GetCurrentTextStyleClass());
	}
}

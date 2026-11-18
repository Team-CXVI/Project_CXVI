// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXSaveSlotMemberWidget.h"

#include <CommonTextBlock.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXSaveSlotMemberWidget)

void UCXSaveSlotMemberWidget::SetMember(const FText& InName, int32 InLevel)
{
	NameText->SetText(InName);
	LevelText->SetText(FText::FormatNamed(LevelFormat, TEXT("Level"), FText::AsNumber(InLevel)));
}

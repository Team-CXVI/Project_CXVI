// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXCombatantEntryWidget.h"

#include <Blueprint/WidgetTree.h>
#include <Components/PanelWidget.h>
#include <Components/Image.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatantEntryWidget)

void UCXCombatantEntryWidget::SetCurrentAP(float InValue)
{
	CurrentAP = FMath::FloorToInt(InValue);
	RefreshAPSlots();
}

void UCXCombatantEntryWidget::SetMaxAP(float InValue)
{
	MaxAP = FMath::Max(0, FMath::FloorToInt(InValue));
	RefreshAPSlots();
}

void UCXCombatantEntryWidget::RefreshAPSlots()
{
	if (nullptr == APSlotPanel)
	{
		return;
	}

	// 최대 AP가 바뀐 경우만 Image 슬롯을 초기화 후 재생성
	if (APSlotImages.Num() != MaxAP)
	{
		APSlotPanel->ClearChildren();
		APSlotImages.Reset();

		for (int32 Index = 0; Index < MaxAP; ++Index)
		{
			UImage* SlotImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			APSlotPanel->AddChild(SlotImage);
			APSlotImages.Emplace(SlotImage);
		}
	}

	for (int32 Index = 0; Index < APSlotImages.Num(); ++Index)
	{
		APSlotImages[Index]->SetBrush(Index < CurrentAP ? FilledSlotBrush : EmptySlotBrush);
	}
}

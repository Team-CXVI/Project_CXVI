// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXPrimaryGameLayout.h"

#include "CXUIManagerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXPrimaryGameLayout)

UCommonActivatableWidgetContainerBase* UCXPrimaryGameLayout::GetLayerWidget(const FGameplayTag& InLayerTag) const
{
	UCommonActivatableWidgetContainerBase* Layer = Layers.FindRef(InLayerTag);
	if (nullptr == Layer)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] 레이어 '%s'가 등록되어 있지 않습니다. 레이아웃 BP의 RegisterLayer를 확인하세요."),
			*GetName(), *InLayerTag.ToString());
	}

	return Layer;
}

void UCXPrimaryGameLayout::FindAndRemoveWidgetFromLayer(UCommonActivatableWidget* InWidget)
{
	if (nullptr == InWidget)
	{
		return;
	}

	for (const TPair<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>>& Pair : Layers)
	{
		if (Pair.Value)
		{
			Pair.Value->RemoveWidget(*InWidget);
		}
	}
}

void UCXPrimaryGameLayout::RegisterLayer(FGameplayTag InLayerTag, UCommonActivatableWidgetContainerBase* InLayerWidget)
{
	// 디자이너 미리보기에서는 등록하지 않음
	if (IsDesignTime())
	{
		return;
	}

	if (!InLayerTag.IsValid())
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] RegisterLayer: 태그가 옮바르지 않습니다."), *GetName());
		return;
	}

	if (nullptr == InLayerWidget)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] RegisterLayer: 위젯이 비어 있습니다."), *GetName());
		return;
	}

	if (Layers.Contains(InLayerTag))
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] 레이어 '%s'가 이미 등록되어 있어 덮어씁니다."), *GetName(), *InLayerTag.ToString());
	}

	Layers.Add(InLayerTag, InLayerWidget);
}

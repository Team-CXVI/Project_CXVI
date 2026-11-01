// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Field/CXPartyMemberEntryWidget.h"

#include <Components/Image.h>
#include <Engine/Texture2D.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXPartyMemberEntryWidget)

void UCXPartyMemberEntryWidget::SetPortrait(TSoftObjectPtr<UTexture2D> InPortrait)
{
	if (nullptr == PortraitImage)
	{
		return;
	}

	// 지정되지 않은 경우 이전에 표시하던 초상화가 남지 않도록 비움
	if (InPortrait.IsNull())
	{
		PortraitImage->SetBrushResourceObject(nullptr);
		return;
	}

	PortraitImage->SetBrushFromSoftTexture(InPortrait);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXCombatAimWidget.h"

#include "Foundation/Combat/CombatMessage/CXCombatMessageData.h"
#include "Tags/CXGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatAimWidget)

void UCXCombatAimWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 메시지를 받기 전까지 숨김
	SetVisibility(ESlateVisibility::Collapsed);

	CommandMenuListener = UCXGameplayMessageSubsystem::Get(this).RegisterListener(CXVIGameplayTags::Message_Combat_CommandMenuChanged, this, &ThisClass::HandleCommandMenuChanged);
}

void UCXCombatAimWidget::NativeDestruct()
{
	CommandMenuListener.Unregister();

	Super::NativeDestruct();
}

void UCXCombatAimWidget::HandleCommandMenuChanged(const FGameplayTag& InChannel, const FCXCombatCommandMenuMessage& InMessage)
{
	if (!InMessage.bMenuActive || !InMessage.bAimMode)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// 조준 중에는 마우스로 시점 이동과 발사를 하므로 자식까지 마우스 입력을 막지 않음
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

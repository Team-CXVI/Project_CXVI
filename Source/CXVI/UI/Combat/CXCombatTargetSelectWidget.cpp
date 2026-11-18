// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXCombatTargetSelectWidget.h"

#include "Character/Combat/CXCombatCharacterPlayer.h"
#include "Foundation/Combat/CombatMessage/CXCombatMessageData.h"
#include "Foundation/GameplayData/Types/CXSkillTypes.h"
#include "Tags/CXGameplayTags.h"
#include "UI/CXUIManagerSubsystem.h"

#include <Components/TextBlock.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatTargetSelectWidget)

void UCXCombatTargetSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 메시지를 받기 전까지 숨김
	SetVisibility(ESlateVisibility::Collapsed);

	CommandMenuListener = UCXGameplayMessageSubsystem::Get(this).RegisterListener(CXVIGameplayTags::Message_Combat_CommandMenuChanged, this, &ThisClass::HandleCommandMenuChanged);
}

void UCXCombatTargetSelectWidget::NativeDestruct()
{
	CommandMenuListener.Unregister();

	Super::NativeDestruct();
}

void UCXCombatTargetSelectWidget::HandleCommandMenuChanged(const FGameplayTag& InChannel, const FCXCombatCommandMenuMessage& InMessage)
{
	const ACXCombatCharacterPlayer* CommandCharacter = InMessage.CommandCharacter.Get();
	if (!InMessage.bMenuActive || InMessage.MenuState != ECommandMenuState::TargetSelect || !IsValid(CommandCharacter))
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	RefreshActionText(InMessage.Selection, CommandCharacter);

	// 전체 화면에 배치되므로 마우스 입력을 막지 않음
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UCXCombatTargetSelectWidget::RefreshActionText(const FCXActionSelection& InSelection, const ACXCombatCharacterPlayer* InCommandCharacter)
{
	FText ActionName = FText::GetEmpty();
	FText Description = FText::GetEmpty();

	switch (InSelection.Type)
	{
	case ECXActionType::BasicAttack:
		ActionName = BasicAttackName;
		Description = BasicAttackDescription;
		break;

	case ECXActionType::Skill:
		if (const FCXSkillRow* SkillRow = InCommandCharacter->GetSkillRowForSlot(InSelection.SlotIndex))
		{
			ActionName = SkillRow->DisplayName;
			Description = SkillRow->Description;
		}
		else
		{
			UE_LOG(LogCXUI, Warning, TEXT("[Target Select] 스킬 행을 찾지 못했습니다. 슬롯=%d (DT_SkillTable 확인)"), InSelection.SlotIndex + 1);
		}
		break;

		// TODO : 전투 아이템 기능(beta/feature/combat-item)이 머지되면 아이템 이름과 설명 표시
	default:
		break;
	}

	ActionNameText->SetText(ActionName);
	DescriptionText->SetText(Description);
}

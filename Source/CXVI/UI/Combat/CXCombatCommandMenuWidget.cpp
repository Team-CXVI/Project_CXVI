// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXCombatCommandMenuWidget.h"

#include "AbilitySystem/Attributes/Combat/CXCombatPlayerAttributeSet.h"
#include "Character/Combat/CXCombatCharacterPlayer.h"
#include "Foundation/GameplayData/CXGameplayDataSubsystem.h"
#include "Foundation/GameplayData/Types/CXItemTypes.h"
#include "Foundation/GameplayData/Types/CXSkillTypes.h"
#include "Foundation/UserData/CXUserDataSubsystem.h"
#include "UI/CXUIManagerSubsystem.h"

#include <AbilitySystemComponent.h>
#include <Components/Border.h>
#include <Components/TextBlock.h>
#include <CommonAnimatedSwitcher.h>
#include <Animation/WidgetAnimation.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatCommandMenuWidget)

void UCXCombatCommandMenuWidget::SetCommandMenu(ECommandMenuState InMenuState, const ACXCombatCharacterPlayer* InCommandCharacter, bool bInOpening)
{
	// 열릴 때 또는 패널이 바뀔 때만 재생. 보이는 중 같은 상태가 다시 오면 재생하지 않음
	const bool bShouldPlayAnimation = bInOpening || InMenuState != LastMenuState;
	LastMenuState = InMenuState;

	switch (InMenuState)
	{
	case ECommandMenuState::ActionMenu:
		MenuSwitcher->SetActiveWidget(ActionPanel);
		break;

	case ECommandMenuState::SkillMenu:
		RefreshSkillSlots(InCommandCharacter);
		MenuSwitcher->SetActiveWidget(SkillPanel);
		break;

	case ECommandMenuState::ItemMenu:
		RefreshItemSlots(InCommandCharacter);
		MenuSwitcher->SetActiveWidget(ItemPanel);
		break;

	default:
		// TargetSelect에서는 메뉴가 표시되지 않음
		break;
	}

	if (bShouldPlayAnimation)
	{
		PlayPanelAnimation(InMenuState);
	}
}

void UCXCombatCommandMenuWidget::PlayPanelAnimation(ECommandMenuState InMenuState)
{
	UWidgetAnimation* PanelAnimation = nullptr;

	switch (InMenuState)
	{
	case ECommandMenuState::ActionMenu:
		PanelAnimation = ActionPanelAnim;
		break;

	case ECommandMenuState::SkillMenu:
		PanelAnimation = SkillPanelAnim;
		break;

	case ECommandMenuState::ItemMenu:
		PanelAnimation = ItemPanelAnim;
		break;

	default:
		break;
	}

	if (PanelAnimation)
	{
		// 재생 중이어도 처음부터 다시 재생
		PlayAnimation(PanelAnimation);
	}
}

void UCXCombatCommandMenuWidget::RefreshSkillSlots(const ACXCombatCharacterPlayer* InCommandCharacter)
{
	UBorder* const SlotTints[] = { Skill1Tint.Get(), Skill2Tint.Get(), Skill3Tint.Get() };
	UTextBlock* const NameTexts[] = { Skill1Text.Get(), Skill2Text.Get(), Skill3Text.Get() };
	UTextBlock* const DescriptionTexts[] = { Skill1Description.Get(), Skill2Description.Get(), Skill3Description.Get() };
	UTextBlock* const CostTexts[] = { Skill1Cost.Get(), Skill2Cost.Get(), Skill3Cost.Get() };

	// 현재 AP. 사용 가능 기준은 스킬 비용 검사(UCXGameplayAbilityCombatBase)와 같음 (AP >= AP 코스트)
	const UAbilitySystemComponent* CommandASC = IsValid(InCommandCharacter) ? InCommandCharacter->GetAbilitySystemComponent() : nullptr;
	const float CurrentAP = CommandASC ? CommandASC->GetNumericAttribute(UCXCombatPlayerAttributeSet::GetAPAttribute()) : 0.f;

	for (int32 SlotIndex = 0; SlotIndex < UE_ARRAY_COUNT(SlotTints); ++SlotIndex)
	{
		const FCXSkillRow* SkillRow = IsValid(InCommandCharacter) ? InCommandCharacter->GetSkillRowForSlot(SlotIndex) : nullptr;

		if (SkillRow)
		{
			NameTexts[SlotIndex]->SetText(SkillRow->DisplayName);
			DescriptionTexts[SlotIndex]->SetText(SkillRow->Description);
			CostTexts[SlotIndex]->SetText(FText::Format(SkillCostFormat, FText::AsNumber(FMath::RoundToInt(SkillRow->APCost))));
		}

		ApplySlotState(SlotTints[SlotIndex], SkillRow != nullptr, SkillRow && CurrentAP >= SkillRow->APCost);
	}
}

void UCXCombatCommandMenuWidget::RefreshItemSlots(const ACXCombatCharacterPlayer* InCommandCharacter)
{
	UBorder* const SlotTints[] = { Item1Tint.Get(), Item2Tint.Get(), Item3Tint.Get() };
	UTextBlock* const NameTexts[] = { Item1Text.Get(), Item2Text.Get(), Item3Text.Get() };
	UTextBlock* const DescriptionTexts[] = { Item1Description.Get(), Item2Description.Get(), Item3Description.Get() };
	UTextBlock* const CountTexts[] = { Item1Count.Get(), Item2Count.Get(), Item3Count.Get() };

	const UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this);
	const bool bHasGameplayData = UCXGameplayDataSubsystem::HasInstance(this);

	for (int32 SlotIndex = 0; SlotIndex < UE_ARRAY_COUNT(SlotTints); ++SlotIndex)
	{
		const FName ItemId = IsValid(InCommandCharacter) ? InCommandCharacter->GetItemIdForSlot(SlotIndex) : NAME_None;
		const FCXItemRow* ItemRow = (!ItemId.IsNone() && bHasGameplayData) ? UCXGameplayDataSubsystem::Get(this).FindItemRow(ItemId) : nullptr;

		// 메뉴 단계에서는 보유 수량만 판단. 대상 HP / AP가 최대인지는 타겟 선택 후 UseItem에서 판단
		const int32 ItemCount = (ItemRow && UserData) ? UserData->GetItemCount(ItemId) : 0;

		if (ItemRow)
		{
			NameTexts[SlotIndex]->SetText(ItemRow->DisplayName);
			// TODO : 아이템 강화 레벨 출처가 정해지면 교체 (지금은 UseItem과 같은 고정값 1)
			constexpr int32 ItemLevel = 1;
			DescriptionTexts[SlotIndex]->SetText(ItemRow->FormatDescription(ItemLevel));
			CountTexts[SlotIndex]->SetText(FText::Format(ItemCountFormat, FText::AsNumber(ItemCount)));
		}
		else if (!ItemId.IsNone())
		{
			UE_LOG(LogCXUI, Warning, TEXT("[Command Menu] DT_Item에 %s 행이 없음 (아이템 %d번 칸 숨김)"), *ItemId.ToString(), SlotIndex + 1);
		}

		ApplySlotState(SlotTints[SlotIndex], ItemRow != nullptr, ItemCount > 0);
	}
}

void UCXCombatCommandMenuWidget::ApplySlotState(UBorder* InSlotTint, bool bInHasData, bool bInUsable) const
{
	if (!bInHasData)
	{
		// Collapsed가 아닌 Hidden : 칸 자리를 유지해서 키 아이콘 순서와 칸 위치가 어긋나지 않게 함
		InSlotTint->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	InSlotTint->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// Border의 Content Color는 안쪽 위젯 전체(프레임 / 글자 / 키 아이콘)의 색에 곱해짐
	InSlotTint->SetContentColorAndOpacity(bInUsable ? FLinearColor::White : UnavailableSlotTint);
}

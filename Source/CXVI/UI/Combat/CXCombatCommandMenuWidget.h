// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Foundation/Combat/CXCombatTypes.h"

#include "CXCombatCommandMenuWidget.generated.h"

class ACXCombatCharacterPlayer;
class UBorder;
class UTextBlock;
class UWidget;
class UCommonAnimatedSwitcher;
class UWidgetAnimation;

/**
 * 전투 Command Menu 화면 (WBP_CombatCommandMenu의 부모, UCXCombatCommandMenuComponent가 표시)
 * - 메뉴 상태에 맞는 패널로 전환하고 패널 내용을 채움
 *   - 스킬 패널 : 칸마다 이름 / 설명 / AP 코스트. 현재 AP < AP 코스트면 비활성 표시
 *   - 아이템 패널 : 칸마다 이름 / 설명 / 보유 수량. 수량 0이면 비활성 표시
 *   - 슬롯 데이터 : ACXCombatCharacterPlayer::GetSkillRowForSlot / GetItemIdForSlot (입력과 같은 기준)
 *   - 비활성 표시 : 칸을 감싼 Border(Skill{N}Tint / Item{N}Tint)의 Content Color로 칸 전체를 어둡게 함
 *   - 데이터가 없는 칸은 Hidden (칸 위치는 유지)
 * - 메뉴가 열릴 때(첫 오픈 / 타겟 선택 취소 후 복귀)와 패널이 바뀔 때 해당 패널의 애니메이션 재생
 *   애니메이션은 WBP의 Animations에 패널별로 만들고 이름을 맞춤 (없으면 재생하지 않음)
 * - 입력은 처리하지 않음. 메뉴 조작은 ACXCommandCharacter의 Enhanced Input과 MenuState가 담당
 */
UCLASS(Abstract)
class CXVI_API UCXCombatCommandMenuWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// bInOpening : 메뉴가 숨겨져 있다가 이번에 표시되면 true
	void SetCommandMenu(ECommandMenuState InMenuState, const ACXCombatCharacterPlayer* InCommandCharacter, bool bInOpening);

private:
	// 스킬 패널 3칸 갱신 (이름 / 설명 / AP 코스트 / 사용 가능 여부)
	void RefreshSkillSlots(const ACXCombatCharacterPlayer* InCommandCharacter);

	// 아이템 패널 3칸 갱신 (이름 / 설명 / 보유 수량 / 사용 가능 여부)
	void RefreshItemSlots(const ACXCombatCharacterPlayer* InCommandCharacter);

	// 칸 표시 상태 적용. 데이터가 없으면 Hidden, 있으면 사용 가능 여부에 따라 칸 색 변경
	void ApplySlotState(UBorder* InSlotTint, bool bInHasData, bool bInUsable) const;

	void PlayPanelAnimation(ECommandMenuState InMenuState);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonAnimatedSwitcher> MenuSwitcher;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ActionPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> SkillPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ItemPanel;

	// 스킬 칸 색 조절용 Border (숨김 / 비활성 색 적용 대상)
	// 패널 애니메이션이 Skill{N}EntryBox의 Render Opacity를 움직이므로, 그 안쪽 Border에 적용
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Skill1Tint;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Skill2Tint;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Skill3Tint;

	// 스킬 이름
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Skill1Text;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Skill2Text;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Skill3Text;

	// 스킬 설명
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Skill1Description;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Skill2Description;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Skill3Description;

	// 스킬 AP 코스트
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Skill1Cost;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Skill2Cost;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Skill3Cost;

	// 아이템 칸 색 조절용 Border (숨김 / 비활성 색 적용 대상)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Item1Tint;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Item2Tint;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Item3Tint;

	// 아이템 이름
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Item1Text;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Item2Text;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Item3Text;

	// 아이템 설명
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Item1Description;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Item2Description;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Item3Description;

	// 아이템 보유 수량
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Item1Count;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Item2Count;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Item3Count;

	// 사용할 수 없는 칸에 곱할 색 (AP 부족 스킬 / 수량 0 아이템). 알파는 1로 두고 RGB로 어둡게 함. 값은 WBP에서 조절
	UPROPERTY(EditAnywhere, Category = "CommandMenu")
	FLinearColor UnavailableSlotTint = FLinearColor(0.35f, 0.35f, 0.35f, 1.f);

	// AP 코스트 표시 형식. {0} = AP 코스트
	UPROPERTY(EditAnywhere, Category = "CommandMenu")
	FText SkillCostFormat = NSLOCTEXT("CXCommandMenu", "SkillCostFormat", "AP {0}");

	// 보유 수량 표시 형식. {0} = 보유 수량
	UPROPERTY(EditAnywhere, Category = "CommandMenu")
	FText ItemCountFormat = NSLOCTEXT("CXCommandMenu", "ItemCountFormat", "x{0}");

	// 패널별 표시 애니메이션 (WBP Animations 이름과 같아야 함)
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> ActionPanelAnim;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> SkillPanelAnim;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> ItemPanelAnim;

	// 마지막으로 표시한 메뉴 상태 (패널 전환 판단용)
	ECommandMenuState LastMenuState = ECommandMenuState::ActionMenu;
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXCombatDefeatWidget.h"

#include "UI/CXButtonBase.h"
#include "UI/CXUITypes.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"
#include "Foundation/UserData/CXUserDataSubsystem.h"
#include "Level/CXLevelTransitionSubsystem.h"
#include "Tags/CXGameplayTags.h"

#include <Kismet/GameplayStatics.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatDefeatWidget)

UCXCombatDefeatWidget::UCXCombatDefeatWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InputMode = ECXWidgetInputMode::Menu;
	DefaultFocusWidgetName = GET_MEMBER_NAME_CHECKED(ThisClass, TitleButton);
}

void UCXCombatDefeatWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	LastPointButton->OnClicked().AddUObject(this, &ThisClass::HandleLastPointClicked);
	LoadButton->OnClicked().AddUObject(this, &ThisClass::HandleLoadClicked);
	TitleButton->OnClicked().AddUObject(this, &ThisClass::HandleTitleClicked);
}

void UCXCombatDefeatWidget::NativeOnActivated()
{
	// 포커스 대상이 활성화 여부로 정해지므로 Super(포커스 대상 확인)보다 먼저 갱신
	RefreshButtons();

	Super::NativeOnActivated();

	UGameplayStatics::SetGamePaused(this, true);

	bMenuReady = false;
	PlayAnimation(IntroAnim);
}

void UCXCombatDefeatWidget::NativeOnDeactivated()
{
	UGameplayStatics::SetGamePaused(this, false);

	bMenuReady = false;

	Super::NativeOnDeactivated();
}

UWidget* UCXCombatDefeatWidget::NativeGetDesiredFocusTarget() const
{
	if (LastPointButton->GetIsEnabled())
	{
		return LastPointButton;
	}

	if (LoadButton->GetIsEnabled())
	{
		return LoadButton;
	}

	return Super::NativeGetDesiredFocusTarget();
}

void UCXCombatDefeatWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* InAnimation)
{
	Super::OnAnimationFinished_Implementation(InAnimation);

	if (InAnimation == IntroAnim.Get())
	{
		bMenuReady = true;
	}
}

void UCXCombatDefeatWidget::RefreshButtons()
{
	const UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this);

	LastPointButton->SetIsEnabled(UserData && UserData->DoesSlotExist(UCXUserDataSubsystem::AutoSaveSlotIndex));
	LoadButton->SetIsEnabled(UserData && UserData->HasAnySaveSlot());
}

void UCXCombatDefeatWidget::HandleLastPointClicked()
{
	if (!bMenuReady)
	{
		return;
	}

	UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this);
	if (nullptr == UserData)
	{
		return;
	}

	// 중복 입력 방지. 실패하면 다른 선택지를 고를 수 있도록 되돌림
	bMenuReady = false;

	// 페이드 동안 전투 화면이 드러나지 않도록 닫지 않음. 레벨이 바뀌면 함께 제거됨
	if (false == UserData->LoadGameAndTravel(UCXUserDataSubsystem::AutoSaveSlotIndex))
	{
		bMenuReady = true;
	}
}

void UCXCombatDefeatWidget::HandleLoadClicked()
{
	if (!bMenuReady)
	{
		return;
	}

	const UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this);
	if (nullptr == LevelTransition || LevelTransition->IsTransitioning())
	{
		return;
	}

	FCXOpenScreenMessage Message;
	Message.ScreenTag = CXVIGameplayTags::UI_Screen_SaveSlot;
	Message.ContextTag = CXVIGameplayTags::UI_ScreenContext_SaveSlot_Load;
	UCXGameplayMessageSubsystem::Get(this).BroadcastMessage(CXVIGameplayTags::Message_UI_OpenScreen, Message);
}

void UCXCombatDefeatWidget::HandleTitleClicked()
{
	if (!bMenuReady)
	{
		return;
	}

	UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this);
	if (nullptr == LevelTransition || LevelTransition->IsTransitioning())
	{
		return;
	}

	// 정지 상태로 이동하지 않도록 먼저 닫아서 정지 해제
	DeactivateWidget();

	LevelTransition->TravelToTitle();
}

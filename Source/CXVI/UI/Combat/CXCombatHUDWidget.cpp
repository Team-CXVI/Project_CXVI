// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXCombatHUDWidget.h"

#include "UI/ViewModel/CXCombatViewModel.h"
#include "UI/Field/CXFieldMenuWidget.h"
#include "Foundation/Combat/CombatMessage/CXCombatMessageData.h"
#include "Tags/CXGameplayTags.h"

#include <Input/CommonUIInputTypes.h>
#include <Components/Border.h>
#include <Components/TextBlock.h>
#include <Animation/WidgetAnimation.h>
#include <MVVMSubsystem.h>
#include <View/MVVMView.h>

// TODO : CXLOG Category 정리 후 대체
#include "UI/CXUIManagerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatHUDWidget)


UCXCombatHUDWidget::UCXCombatHUDWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InputMode = ECXWidgetInputMode::GameAndMenu;
}

void UCXCombatHUDWidget::SetCombatViewModel(UCXCombatViewModel* InViewModel)
{
	UMVVMView* View = UMVVMSubsystem::GetViewFromUserWidget(this);
	if (nullptr == View || false == View->SetViewModelByClass(InViewModel))
	{
		UE_LOG(LogCXUI, Error, TEXT("%s : CombatViewModel을 연결하지 못했습니다. WBP Viewmodels에 CXCombatViewModel(Manual)이 있는지, View Binding이 1개 이상 있는지 확인하세요."), *GetName());
	}
}

void UCXCombatHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 메시지를 받기 전까지 숨김
	HideSystemMessage();

	ActionStartedListener = UCXGameplayMessageSubsystem::Get(this).RegisterListener(CXVIGameplayTags::Message_Combat_ActionStarted, this, &ThisClass::HandleActionStarted);
	CommandMenuListener = UCXGameplayMessageSubsystem::Get(this).RegisterListener(CXVIGameplayTags::Message_Combat_CommandMenuChanged, this, &ThisClass::HandleCommandMenuChanged);

	// 전투 시작 전(메시지 전)은 뒤로가기 상태가 아니므로 메뉴 열기 가능
	SetOpenMenuBindingActive(true);
}

void UCXCombatHUDWidget::NativeDestruct()
{
	ActionStartedListener.Unregister();
	CommandMenuListener.Unregister();

	SetOpenMenuBindingActive(false);

	Super::NativeDestruct();
}

void UCXCombatHUDWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* InAnimation)
{
	Super::OnAnimationFinished_Implementation(InAnimation);

	// 다시 재생 중이면(새 메시지로 재시작) 숨기지 않음
	if (InAnimation == SystemMessageAnim && false == IsAnimationPlaying(SystemMessageAnim))
	{
		HideSystemMessage();
	}
}

void UCXCombatHUDWidget::HandleActionStarted(const FGameplayTag& InChannel, const FCXCombatActionStartedMessage& InMessage)
{
	// 기본 : 스킬 이름만 표시. 기본 공격 / 반격 문구는 정해지지 않아 표시하지 않음
	if (ECXActionType::Skill != InMessage.ActionType || InMessage.ActionName.IsEmpty())
	{
		return;
	}

	ShowSystemMessage(InMessage.ActionName);
}

void UCXCombatHUDWidget::ShowSystemMessage(const FText& InText)
{
	if (nullptr == SystemMessageBox || nullptr == SystemMessageText)
	{
		return;
	}

	// 애니메이션이 없으면 숨길 시점이 없으므로 표시하지 않음
	if (nullptr == SystemMessageAnim)
	{
		UE_LOG(LogCXUI, Warning, TEXT("%s : SystemMessageAnim이 없어 시스템 메시지를 표시하지 않습니다. WBP에 SystemMessageAnim 애니메이션을 추가하세요."), *GetName());
		return;
	}

	SystemMessageText->SetText(InText);
	SystemMessageBox->SetVisibility(ESlateVisibility::HitTestInvisible);

	// 표시 중이어도 처음부터 다시 재생
	PlayAnimation(SystemMessageAnim, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f, true);
}

void UCXCombatHUDWidget::HideSystemMessage()
{
	if (SystemMessageBox)
	{
		SystemMessageBox->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCXCombatHUDWidget::HandleCommandMenuChanged(const FGameplayTag& InChannel, const FCXCombatCommandMenuMessage& InMessage)
{
	// 메뉴를 열지 않는 상태
	// - ESC가 뒤로가기(IA_CB_Cancel)로 쓰이는 상태 : 스킬 / 아이템 메뉴, 대상 선택
	// - 조준 모드 : ESC로 메뉴를 열지 않음 (조준 해제는 IA_CB_AimMode)
	const bool bBlockOpenMenu = InMessage.bMenuActive
		&& (InMessage.bAimMode
			|| ECommandMenuState::SkillMenu == InMessage.MenuState
			|| ECommandMenuState::ItemMenu == InMessage.MenuState
			|| ECommandMenuState::TargetSelect == InMessage.MenuState);

	SetOpenMenuBindingActive(false == bBlockOpenMenu);
}

void UCXCombatHUDWidget::SetOpenMenuBindingActive(bool bInActive)
{
	if (OpenMenuBinding.IsValid() == bInActive)
	{
		return;
	}

	if (bInActive)
	{
		if (nullptr == OpenMenuAction)
		{
			UE_LOG(LogCXUI, Warning, TEXT("[%s] OpenMenuAction이 지정되지 않았습니다."), *GetName());
			return;
		}

		// 메뉴가 열리면 Menu 입력 모드가 되어 이 바인딩은 동작하지 않고, ESC는 메뉴의 뒤로가기로 처리됨
		FBindUIActionArgs BindArgs(OpenMenuAction, /*bShouldDisplayInActionBar*/ false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleOpenMenu));
		BindArgs.InputMode = ECommonInputMode::Game;

		OpenMenuBinding = RegisterUIActionBinding(BindArgs);
		return;
	}

	// RemoveActionBinding : 위젯과 입력 처리 목록에서 제거 / Unregister : 바인딩 자체를 삭제
	RemoveActionBinding(OpenMenuBinding);
	OpenMenuBinding.Unregister();
	OpenMenuBinding = FUIActionBindingHandle();
}

void UCXCombatHUDWidget::HandleOpenMenu()
{
	if (nullptr == MenuClass)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] MenuClass가 지정되지 않았습니다."), *GetName());
		return;
	}

	UCXUIManagerSubsystem* UIManager = UCXUIManagerSubsystem::TryGet(this);
	if (nullptr == UIManager)
	{
		return;
	}

	UIManager->PushWidgetToLayer<UCXFieldMenuWidget>(CXVIGameplayTags::UI_Layer_GameMenu, MenuClass);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Field/CXFieldHUDWidget.h"

#include "CXFieldMenuWidget.h"
#include "UI/CXUIManagerSubsystem.h"
#include "Tags/CXGameplayTags.h"
#include "Foundation/Field/FieldMessage/CXFieldMessageData.h"
#include "Foundation/GameplayData/CXGameplayDataSubsystem.h"

#include <Input/CommonUIInputTypes.h>
#include <Components/Border.h>
#include <Components/TextBlock.h>
#include <Animation/WidgetAnimation.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldHUDWidget)

UCXFieldHUDWidget::UCXFieldHUDWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InputMode = ECXWidgetInputMode::Game;
}

void UCXFieldHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (nullptr == OpenMenuAction)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] OpenMenuAction이 지정되지 않았습니다."), *GetName());
		return;
	}

	// 메뉴 열기 InputAction은 UI에서 바인딩 -> CommonUI에서 처리 
	// 입력은 InputMode가 Game 모드에서 받도록 지정
	FBindUIActionArgs BindArgs(OpenMenuAction, /*bShouldDisplayInActionBar*/ false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleOpenMenu));
	BindArgs.InputMode = ECommonInputMode::Game;

	RegisterUIActionBinding(BindArgs);
}

void UCXFieldHUDWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	// 메시지를 받기 전까지 숨김
	HideSystemMessage();

	ItemAcquiredListener = UCXGameplayMessageSubsystem::Get(this).RegisterListener(CXVIGameplayTags::Message_Field_ItemAcquired, this, &ThisClass::HandleItemAcquired);

	const UCXUIManagerSubsystem* UIManager = UCXUIManagerSubsystem::TryGet(this);
	const UCXPrimaryGameLayout* PrimaryLayout = UIManager ? UIManager->GetPrimaryLayout() : nullptr;
	UCommonActivatableWidgetContainerBase* Layer = PrimaryLayout ? PrimaryLayout->GetLayerWidget(CXVIGameplayTags::UI_Layer_GameMenu) : nullptr;
	if (nullptr == Layer)
	{
		return;
	}

	GameMenuLayer = Layer;
	GameMenuLayerChangedHandle = Layer->OnDisplayedWidgetChanged().AddUObject(this, &ThisClass::HandleGameMenuLayerChanged);
}

void UCXFieldHUDWidget::NativeOnDeactivated()
{
	ItemAcquiredListener.Unregister();

	// 풀에서 재사용될 때 이전 메시지가 남지 않도록 정리
	if (SystemMessageAnim)
	{
		StopAnimation(SystemMessageAnim);
	}
	HideSystemMessage();

	if (UCommonActivatableWidgetContainerBase* Layer = GameMenuLayer.Get())
	{
		Layer->OnDisplayedWidgetChanged().Remove(GameMenuLayerChangedHandle);
	}

	GameMenuLayer.Reset();
	GameMenuLayerChangedHandle.Reset();

	// 풀에서 재사용될 때 숨겨진 채로 나오지 않도록 복원
	SetContentHidden(false);

	Super::NativeOnDeactivated();
}

void UCXFieldHUDWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* InAnimation)
{
	Super::OnAnimationFinished_Implementation(InAnimation);

	// 다시 재생 중이면(새 메시지로 재시작) 숨기지 않음
	if (InAnimation == SystemMessageAnim && false == IsAnimationPlaying(SystemMessageAnim))
	{
		HideSystemMessage();
	}
}

void UCXFieldHUDWidget::HandleOpenMenu()
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

void UCXFieldHUDWidget::HandleGameMenuLayerChanged(UCommonActivatableWidget* InDisplayedWidget)
{
	SetContentHidden(nullptr != InDisplayedWidget);
}

void UCXFieldHUDWidget::SetContentHidden(bool bInHidden)
{
	// 이미 숨긴 상태에서 다시 저장하면 Collapsed가 원래 값으로 저장되므로 상태가 바뀔 때만 처리
	if (bContentHidden == bInHidden)
	{
		return;
	}

	UWidget* RootWidget = GetRootWidget();
	if (nullptr == RootWidget)
	{
		return;
	}

	bContentHidden = bInHidden;

	if (bInHidden)
	{
		CachedRootVisibility = RootWidget->GetVisibility();
		RootWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		RootWidget->SetVisibility(CachedRootVisibility);
	}
}

void UCXFieldHUDWidget::HandleItemAcquired(const FGameplayTag& InChannel, const FCXItemAcquiredMessage& InMessage)
{
	const UCXGameplayDataSubsystem& GameplayData = UCXGameplayDataSubsystem::Get(this);

	// 아이템마다 한 줄씩 만들어 줄바꿈으로 이어 붙임
	TArray<FText> Lines;
	for (const FCXItemStack& Item : InMessage.Items)
	{
		const FCXItemRow* ItemRow = GameplayData.FindItemRow(Item.ItemId);
		if (nullptr == ItemRow)
		{
			UE_LOG(LogCXUI, Warning, TEXT("%s : DT_Item에 %s Row가 없어 습득 메시지에서 제외합니다."), *GetName(), *Item.ItemId.ToString());
			continue;
		}

		FFormatNamedArguments Args;
		Args.Add(TEXT("Name"), ItemRow->DisplayName);
		Args.Add(TEXT("Count"), FText::AsNumber(Item.Count));
		Lines.Add(FText::Format(ItemAcquiredFormat, Args));
	}

	if (Lines.IsEmpty())
	{
		return;
	}

	ShowSystemMessage(FText::Join(FText::FromString(TEXT("\n")), Lines));
}

void UCXFieldHUDWidget::ShowSystemMessage(const FText& InText)
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

void UCXFieldHUDWidget::HideSystemMessage()
{
	if (SystemMessageBox)
	{
		SystemMessageBox->SetVisibility(ESlateVisibility::Collapsed);
	}
}

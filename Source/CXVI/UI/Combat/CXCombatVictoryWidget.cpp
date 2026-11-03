// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXCombatVictoryWidget.h"

#include "UI/CXInputHintWidget.h"
#include "UI/CXUIManagerSubsystem.h"

#include <Input/CommonUIInputTypes.h>
#include <Kismet/GameplayStatics.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXTurnOrderWidget)

UCXCombatVictoryWidget::UCXCombatVictoryWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	// 포커스가 필요 없는 화면. 일시정지 중이라 게임 입력은 동작하지 않음
	InputMode = ECXWidgetInputMode::GameAndMenu;
}

void UCXCombatVictoryWidget::SetOnConfirmed(FSimpleDelegate InOnConfirmed)
{
	OnConfirmed = MoveTemp(InOnConfirmed);
}

void UCXCombatVictoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (nullptr == ConfirmAction)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] ConfirmAction이 지정되지 않았습니다."), *GetName());
		return;
	}

	FBindUIActionArgs BindArgs(ConfirmAction, /*bShouldDisplayInActionBar*/ false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleConfirm));

	RegisterUIActionBinding(BindArgs);
}

void UCXCombatVictoryWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	UGameplayStatics::SetGamePaused(this, true);

	bCanConfirm = false;
	ConfirmHint->SetVisibility(ESlateVisibility::Collapsed);

	PlayAnimation(FrameEnterAnim);
}

void UCXCombatVictoryWidget::NativeOnDeactivated()
{
	UGameplayStatics::SetGamePaused(this, false);

	// 외부에서 닫힌 경우에도 재사용될 때 이전 콜백이 남지 않도록 정리
	bCanConfirm = false;
	OnConfirmed.Unbind();

	Super::NativeOnDeactivated();
}

void UCXCombatVictoryWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* InAnimation)
{
	Super::OnAnimationFinished_Implementation(InAnimation);

	if (InAnimation != FrameEnterAnim.Get())
	{
		return;
	}

	ConfirmHint->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	bCanConfirm = true;
}

void UCXCombatVictoryWidget::HandleConfirm()
{
	if (!bCanConfirm)
	{
		return;
	}

	// NativeOnDeactivated에서 콜백을 비우므로 먼저 옮겨 둠
	const FSimpleDelegate Action = MoveTemp(OnConfirmed);

	DeactivateWidget();

	Action.ExecuteIfBound();
}

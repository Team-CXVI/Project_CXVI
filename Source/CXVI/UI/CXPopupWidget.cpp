// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXPopupWidget.h"

#include "UI/CXButtonBase.h"

#include <Animation/WidgetAnimation.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXPopupWidget)

UCXPopupWidget::UCXPopupWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InputMode = ECXWidgetInputMode::Menu;
	DefaultFocusWidgetName = GET_MEMBER_NAME_CHECKED(ThisClass, ConfirmButton);

	// 뒤로 가기 입력처리를 활성화
	bIsBackHandler = true;
}

void UCXPopupWidget::SetupConfirm(FName InBodyTextId, FSimpleDelegate InOnConfirm)
{
	SetBodyTextById(InBodyTextId);

	OnConfirmAction = MoveTemp(InOnConfirm);
	OnCancelAction.Unbind();

	CancelButton->SetVisibility(ESlateVisibility::Collapsed);
}

void UCXPopupWidget::SetupConfirmCancel(FName InBodyTextId, FSimpleDelegate InOnConfirm, FSimpleDelegate InOnCancel)
{
	SetBodyTextById(InBodyTextId);

	OnConfirmAction = MoveTemp(InOnConfirm);
	OnCancelAction = MoveTemp(InOnCancel);

	CancelButton->SetVisibility(ESlateVisibility::Visible);
}

void UCXPopupWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ConfirmButton->OnClicked().AddUObject(this, &ThisClass::HandleConfirmClicked);
	CancelButton->OnClicked().AddUObject(this, &ThisClass::HandleCancelClicked);
}

void UCXPopupWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	bIsClosing = false;
	PlayAnimation(PopupAnim);
}

void UCXPopupWidget::NativeOnDeactivated()
{
	// 외부에서 닫힌 경우(RemoveWidget 등)에도 재사용될 때 이전 콜백이 남지 않도록 정리
	bIsClosing = false;
	OnConfirmAction.Unbind();
	OnCancelAction.Unbind();
	PendingAction.Unbind();

	Super::NativeOnDeactivated();
}

bool UCXPopupWidget::NativeOnHandleBackAction()
{
	BeginClose(CancelButton->IsVisible() ? OnCancelAction : OnConfirmAction);

	// 기본 구현은 즉시 닫으므로 Super를 호출하지 않음
	return true;
}

void UCXPopupWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* InAnimation)
{
	Super::OnAnimationFinished_Implementation(InAnimation);

	if (!bIsClosing || InAnimation != PopupAnim.Get())
	{
		return;
	}

	// NativeOnDeactivated에서 PendingAction을 비우므로 먼저 옮겨 둠
	const FSimpleDelegate Action = MoveTemp(PendingAction);

	DeactivateWidget();

	Action.ExecuteIfBound();
}

void UCXPopupWidget::SetBodyTextById(FName InBodyTextId)
{
	// TODO : 텍스트 데이터 테이블 작업 때 InBodyTextId로 본문을 찾아 BodyTextBlock에 설정
}

void UCXPopupWidget::HandleConfirmClicked()
{
	BeginClose(OnConfirmAction);
}

void UCXPopupWidget::HandleCancelClicked()
{
	BeginClose(OnCancelAction);
}

void UCXPopupWidget::BeginClose(FSimpleDelegate& InAction)
{
	if (bIsClosing)
	{
		return;
	}

	bIsClosing = true;
	PendingAction = MoveTemp(InAction);

	OnConfirmAction.Unbind();
	OnCancelAction.Unbind();

	// 등장 중이면 그 위치에서 방향만 바뀜
	PlayAnimationReverse(PopupAnim);
}

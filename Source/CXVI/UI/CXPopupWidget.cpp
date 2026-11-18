// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXPopupWidget.h"

#include "UI/CXButtonBase.h"
#include "UI/CXUIManagerSubsystem.h"

#include <Animation/WidgetAnimation.h>
#include <CommonTextBlock.h>
#include <Internationalization/StringTable.h>
#include <Internationalization/StringTableCore.h>

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
	// 위젯이 재사용되므로 찾지 못한 경우에도 이전 본문이 남지 않게 먼저 비움
	BodyTextBlock->SetText(FText::GetEmpty());

	// 본문 없이 띄우는 팝업 (디버그 명령 등)
	if (InBodyTextId.IsNone())
	{
		return;
	}

	if (nullptr == SystemMessageTable)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[Popup] SystemMessageTable이 지정되지 않았습니다. (WBP_Popup Class Defaults 확인) 본문 ID=%s"), *InBodyTextId.ToString());
		return;
	}

	const FString Key = InBodyTextId.ToString();
	if (!SystemMessageTable->GetStringTable()->FindEntry(Key).IsValid())
	{
		UE_LOG(LogCXUI, Warning, TEXT("[Popup] 시스템 메시지 키를 찾지 못했습니다. 키=%s (%s 확인)"), *Key, *SystemMessageTable->GetName());
		return;
	}

	BodyTextBlock->SetText(FText::FromStringTable(SystemMessageTable->GetStringTableId(), Key));
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

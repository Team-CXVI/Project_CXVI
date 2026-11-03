// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/CXActivatableWidget.h"

#include "CXPopupWidget.generated.h"

class UCXButtonBase;
class UCommonTextBlock;
class UWidgetAnimation;

/**
 * 공용 팝업 (WBP_Popup의 부모)
 * - UCXUIManagerSubsystem의 ShowConfirmPopup / ShowConfirmCancelPopup이 UI.Layer.Modal에 Push
 * - 입력 모드 Menu. 뒤쪽 마우스 입력은 WBP의 DimPanel이 차단
 * - 버튼 1개(확인) / 2개(왼쪽 확인, 오른쪽 취소). 콜백이 없는 버튼은 닫기만 함
 * - 뒤로가기는 버튼 1개면 확인, 2개면 취소와 동일
 * - 활성화 시 PopupAnim을 재생, 닫을 때 역재생. 역재생이 끝나면 닫은 뒤 콜백 실행
 *   (콜백이 다음 팝업을 열 수 있으므로 닫기가 먼저)
 * - Stack / Queue가 위젯을 재사용하므로 콜백과 닫기 상태는 비활성화될 때 정리
 */
UCLASS(Abstract)
class CXVI_API UCXPopupWidget : public UCXActivatableWidget
{
	GENERATED_BODY()
	
public:
	UCXPopupWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 버튼 1개 구성
	void SetupConfirm(FName InBodyTextId, FSimpleDelegate InOnConfirm);

	// 버튼 2개 구성
	void SetupConfirmCancel(FName InBodyTextId, FSimpleDelegate InOnConfirm, FSimpleDelegate InOnCancel);

protected:
	virtual void NativeOnInitialized() override;

	virtual void NativeOnActivated() override;

	virtual void NativeOnDeactivated() override;

	virtual bool NativeOnHandleBackAction() override;

	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* InAnimation) override;

private:
	void SetBodyTextById(FName InBodyTextId);

	void HandleConfirmClicked();

	void HandleCancelClicked();

	// 실행할 콜백을 보관하고 역재생 시작. 이미 닫는 중이면 무시
	void BeginClose(FSimpleDelegate& InAction);

protected:
	// 본문
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> BodyTextBlock;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXButtonBase> ConfirmButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXButtonBase> CancelButton;

	// 등장 애니메이션. 닫을 때 역재생
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> PopupAnim;

private:
	FSimpleDelegate OnConfirmAction;

	FSimpleDelegate OnCancelAction;

	// 역재생이 끝난 뒤 실행할 콜백
	FSimpleDelegate PendingAction;

	bool bIsClosing = false;
	
};

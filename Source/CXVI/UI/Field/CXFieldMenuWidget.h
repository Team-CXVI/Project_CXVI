// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/CXActivatableWidget.h"

#include "CXFieldMenuWidget.generated.h"

class UCXButtonBase;

/**
 * 필드 메뉴 (WBP_FieldMenu의 부모)
 * - 필드 HUD가 메뉴 입력을 받으면 UI.Layer.GameMenu에 Push
 * - 입력 모드 Menu (게임 입력 차단, 조작 방식은 타이틀 화면과 동일)
 * - 활성화 동안 게임 일시정지 (Activated → 정지 / Deactivated → 해제)
 * - 계속하기 : 닫기 (뒤로가기도 같은 동작)
 * - 타이틀로 돌아가기 : 닫아서 정지 해제 후 타이틀 이동
 *   TODO : 동작 검증이 필요, 타이틀 이동 사이에 플레이어 입력 위험성
 * - 게임 종료하기 : 게임 종료
 * - 첫 포커스는 DefaultFocusWidgetName (WBP에서 ContinueButton 지정)
 */
UCLASS(Abstract)
class CXVI_API UCXFieldMenuWidget : public UCXActivatableWidget
{
	GENERATED_BODY()
	
public:
	UCXFieldMenuWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativeOnInitialized() override;

	virtual void NativeOnActivated() override;

	virtual void NativeOnDeactivated() override;

private:
	void HandleContinueClicked();

	void HandleTitleClicked();

	void HandleQuitClicked();

protected:
	// 계속하기 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXButtonBase> ContinueButton;

	// 타이틀로 돌아가기 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXButtonBase> TitleButton;
	
	// 게임 닫기 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXButtonBase> QuitButton;
};

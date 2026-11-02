// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/CXActivatableWidget.h"

#include "CXTitleScreenWidget.generated.h"

class UCXButtonBase;

/**
 * 타이틀 화면
 * - 게임 시작 : 새 게임 레벨(Settings.NewGameLevel)로 이동
 * - 이어하기 : 저장 슬롯 목록을 불러오기로 열기. 저장 파일이 없으면 비활성화 (활성화될 때마다 확인)
 * - 게임 종료 : 게임 종료
 * 
 * - 메뉴 이동 입력은 Slate 내비게이션 기준 (W/S, 방향키, D-Pad).
 * - 순환과 좌우 정지는 WBP 메뉴 박스의 Navigation 규칙으로 설정
 * - 확인(Space, Enter, 게임패드 A)은 포커스된 버튼을 클릭 (CommonButtonAcceptKeyHandling=TriggerClick)
 * - 첫 포커스는 DefaultFocusWidgetName (WBP에서 StartButton 지정)
 */
UCLASS(Abstract)
class CXVI_API UCXTitleScreenWidget : public UCXActivatableWidget
{
	GENERATED_BODY()
	
public:
	UCXTitleScreenWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativeOnInitialized() override;

	virtual void NativeOnActivated() override;

private:
	void HandleStartClicked();

	void HandleContinueClicked();

	void HandleQuitClicked();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXButtonBase> StartButton;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXButtonBase> ContinueButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXButtonBase> QuitButton;
};

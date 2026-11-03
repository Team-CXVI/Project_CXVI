// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/CXActivatableWidget.h"

#include "CXFlagMenuWidget.generated.h"

class UCXButtonBase;

/**
 * 깃발 메뉴 (WBP_FlagMenu의 부모)
 * - 플레이어가 깃발과 상호작용하면 Message.UI.OpenScreen의 UI.Screen.FlagMenu로 UI.Layer.GameMenu에 열림
 * - 입력 모드 Menu (게임 입력 차단)
 * - 활성화 동안 게임 일시정지 (Activated → 정지 / Deactivated → 해제)
 * - 휴식하기 : 메뉴는 닫지 않음 (파티 상태창에서 회복 확인)
 * - 저장하기 : 저장 슬롯 목록을 저장 모드로 열기 (UI.Layer.Menu에 열리므로 이 메뉴는 활성 상태 유지)
 * - 나가기 : 닫기 (뒤로가기도 같은 동작)
 * - 파티 상태창은 WBP에서 전역 PartyViewModel을 바인딩 (C++ 처리 없음)
 * - 첫 포커스는 DefaultFocusWidgetName (WBP에서 RestButton 지정)
 */
UCLASS(Abstract)
class CXVI_API UCXFlagMenuWidget : public UCXActivatableWidget
{
	GENERATED_BODY()
	
public:
	UCXFlagMenuWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativeOnInitialized() override;

	virtual void NativeOnActivated() override;

	virtual void NativeOnDeactivated() override;
	
private:
	void HandleRestClicked();

	void HandleSaveClicked();

	void HandleExitClicked();

protected:
	// 휴식하기 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXButtonBase> RestButton;

	// 저장하기 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXButtonBase> SaveButton;

	// 나가기 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXButtonBase> ExitButton;
};

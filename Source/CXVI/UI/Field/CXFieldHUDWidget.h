// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/CXActivatableWidget.h"

#include "CXFieldHUDWidget.generated.h"

class UInputAction;
class UCXFieldMenuWidget;
class UCommonActivatableWidget;
class UCommonActivatableWidgetContainerBase;

/**
 * 필드 HUD 베이스 (WBP_FieldHUD의 부모)
 * - 필드 Scene 정의의 HUDClass로 지정 → Scene이 UI.Layer.Game에 Push
 * - 입력 모드 Game
 * - 메뉴 열기 : OpenMenuAction을 Game 모드 UI 액션으로 바인딩 → UI.Layer.GameMenu에 MenuClass Push
 *   (OpenMenuAction의 키는 IMC_UI에 매핑해야 키 조회 / 입력 처리가 됨)
 * - 버튼 안내(메뉴 / 이동 / 시점 / 공격)는 WBP에서 WBP_InputHint로 배치하고 키를 지정
 * - 메뉴가 열린 동안 숨김 : UI.Layer.GameMenu의 표시 위젯 변경을 구독해서 루트 위젯을 Collapsed / 복원
 *   (HUD 자신을 숨기면 CommonUI가 입력 불가로 처리하므로 루트 위젯만 숨김)
 */
UCLASS(Abstract)
class CXVI_API UCXFieldHUDWidget : public UCXActivatableWidget
{
	GENERATED_BODY()
	
public:
	UCXFieldHUDWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativeOnInitialized() override;

	virtual void NativeOnActivated() override;

	virtual void NativeOnDeactivated() override;

private:
	void HandleOpenMenu();
	
	// GameMenu 레이어의 표시 위젯이 바뀔 때 호출. 표시 중인 위젯이 없으면 nullptr
	void HandleGameMenuLayerChanged(UCommonActivatableWidget* InDisplayedWidget);

	void SetContentHidden(bool bInHidden);

protected:
	// 메뉴 열기 입력
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TObjectPtr<UInputAction> OpenMenuAction;

	// 메뉴 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TSubclassOf<UCXFieldMenuWidget> MenuClass;


private:
	// 구독 중인 GameMenu 레이어
	TWeakObjectPtr<UCommonActivatableWidgetContainerBase> GameMenuLayer;

	FDelegateHandle GameMenuLayerChangedHandle;

	// 숨기기 전 루트 위젯의 Visibility (복원용)
	ESlateVisibility CachedRootVisibility = ESlateVisibility::Visible;

	bool bContentHidden = false;
};

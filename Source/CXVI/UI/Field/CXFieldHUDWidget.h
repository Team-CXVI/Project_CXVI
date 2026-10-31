// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/CXActivatableWidget.h"

#include "CXFieldHUDWidget.generated.h"

class UInputAction;
class UCXFieldMenuWidget;

/**
 * 필드 HUD 베이스 (WBP_FieldHUD의 부모)
 * - 필드 Scene 정의의 HUDClass로 지정 → Scene이 UI.Layer.Game에 Push
 * - 입력 모드 Game
 * - 메뉴 열기 : OpenMenuAction을 Game 모드 UI 액션으로 바인딩 → UI.Layer.GameMenu에 MenuClass Push
 *   (OpenMenuAction의 키는 IMC_UI에 매핑해야 키 조회 / 입력 처리가 됨)
 * - 버튼 안내(메뉴 / 이동 / 시점 / 공격)는 WBP에서 WBP_InputHint로 배치하고 키를 지정
 */
UCLASS(Abstract)
class CXVI_API UCXFieldHUDWidget : public UCXActivatableWidget
{
	GENERATED_BODY()
	
public:
	UCXFieldHUDWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativeOnInitialized() override;

private:
	void HandleOpenMenu();
	
protected:
	// 메뉴 열기 입력
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TObjectPtr<UInputAction> OpenMenuAction;

	// 메뉴 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TSubclassOf<UCXFieldMenuWidget> MenuClass;
};

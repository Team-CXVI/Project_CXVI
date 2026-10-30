// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/CXActivatableWidget.h"

#include "CXCombatHUDWidget.generated.h"

class UCXCombatViewModel;

/**
 * 전투 HUD 베이스 (WBP_CombatHUD의 부모)
 * - 전투 Scene 정의의 HUDClass로 지정 → Scene이 UI.Layer.Game에 Push
 * - 입력 모드 GameAndMenu (전투 입력 + UI 입력)
 * - SetCombatViewModel : WBP에 선언한 CXCombatViewModel Viewmodel(생성 방식 Manual)에 주입
 * - 파티 패널 / 보스 HP UI 배치와 바인딩은 WBP의 View Bindings에서
 */
UCLASS(Abstract)
class CXVI_API UCXCombatHUDWidget : public UCXActivatableWidget
{
	GENERATED_BODY()
	
	
public:
	UCXCombatHUDWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void SetCombatViewModel(UCXCombatViewModel* InViewModel);
	
};

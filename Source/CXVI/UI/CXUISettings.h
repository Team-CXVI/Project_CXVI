// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"

#include "CXUISettings.generated.h"

// Message.UI.OpenScreen으로 여는 화면 하나의 설정
USTRUCT()
struct FCXScreenEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Screen")
	TSoftClassPtr<class UCXActivatableWidget> WidgetClass;

	// Push할 레이어
	UPROPERTY(EditAnywhere, Category = "Screen", meta = (Categories = "UI.Layer"))
	FGameplayTag LayerTag;
};


/**
 * UI 루트 레이아웃 / UI 공통 입력 설정
 * Project Settings > Game > CXVI UI
 * 저장 위치 : Config/DefaultGame.ini [/Script/CXVI.CXUISettings]
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "CXVI UI"))
class CXVI_API UCXUISettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UCXUISettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	// 로컬 플레이어마다 생성하는 루트 레이아웃 (W_PrimaryGameLayout)
	UPROPERTY(Config, EditAnywhere, Category = "Layout")
	TSoftClassPtr<class UCXPrimaryGameLayout> PrimaryLayoutClass;

	UPROPERTY(Config, EditAnywhere, Category = "Layout")
	int32 LayoutZOrder = 1000;

	// 레이아웃이 있는 동안 항상 적용하는 UI 공통 입력 (확인, 뒤로가기, 탭 전환)
	// CommonUI가 IA에 매핑된 키를 찾으려면 활성 IMC에 들어 있어야 함
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	TSoftObjectPtr<class UInputMappingContext> UIMappingContext;

	// 같은 키가 게임 IMC에도 있을 때 UI 쪽이 먼저 처리하도록 높게 설정
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	int32 UIMappingPriority = 100;
	
	// ShowConfirmPopup / ShowConfirmCancelPopup으로 여는 공용 팝업 (WBP_Popup)
	UPROPERTY(Config, EditAnywhere, Category = "Popup")
	TSoftClassPtr<class UCXPopupWidget> PopupClass;

	// Message.UI.OpenScreen으로 열 수 있는 화면 (화면 태그 → 위젯, 레이어)
	UPROPERTY(Config, EditAnywhere, Category = "Screen", meta = (ForceInlineRow, Categories = "UI.Screen"))
	TMap<FGameplayTag, FCXScreenEntry> Screens;
};

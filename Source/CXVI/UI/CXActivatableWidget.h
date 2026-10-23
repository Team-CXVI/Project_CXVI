// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "CXActivatableWidget.generated.h"

UENUM(BlueprintType)
enum class ECXWidgetInputMode : uint8
{
	// 입력 모드를 바꾸지 않음 (아래 화면의 설정 유지)
	Default,

	// 게임 입력 + UI 입력 (전투 HUD 등)
	GameAndMenu,

	// 게임 입력만 (필드 HUD)
	Game,

	// UI 입력만, 게임 입력 차단 (메뉴, 팝업)
	Menu
};

/**
 * 모든 화면의 베이스
 * - InputMode로 활성화 시 입력 모드를 선언 (맨 위의 ActivatableWidget 기준으로 적용됨)
 * - 게임패드용 첫 포커스: BP의 Get Desired Focus Target 구현 → 없으면 DefaultFocusWidgetName 위젯
 * - Menu 화면은 BP에서 Is Back Handler를 켜면 뒤로가기로 자동 닫힘
 */
UCLASS(abstract, Blueprintable)
class CXVI_API UCXActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()
	
public:
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	// 활성화 될 때마다 호출되는 함수 - 한번만 초기화되야하는 로직은 NativeOnInitialized로
	virtual void NativeOnActivated() override;

#if WITH_EDITOR
	virtual void ValidateCompiledWidgetTree(const UWidgetTree& BlueprintWidgetTree, IWidgetCompilerLog& CompileLog) const override;
#endif

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	ECXWidgetInputMode InputMode = ECXWidgetInputMode::Default;

	// Game / GameAndMenu일 때 마우스 캡쳐(마우스와 상호 작용) 방식
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	EMouseCaptureMode GameMouseCaptureMode = EMouseCaptureMode::CapturePermanently;
	
	// 활성화 시 포커스를 줄 위젯 이름 (디자이너 편집의 위젯 이름)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FName DefaultFocusWidgetName;
};

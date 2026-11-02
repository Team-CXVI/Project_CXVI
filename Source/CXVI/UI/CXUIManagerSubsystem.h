// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "CXPrimaryGameLayout.h"
#include "GameplayTagContainer.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"
#include <Containers/Ticker.h>

#include "CXUIManagerSubsystem.generated.h"

class APlayerController;
struct FCXNavigationConfig;
struct FCXScreenFadeMessage;
struct FCXOpenScreenMessage;

DECLARE_LOG_CATEGORY_EXTERN(LogCXUI, Log, All);


/**
 * UI 매니저 (로컬 플레이어마다 하나)
 * - 기능
 * 루트 레이아웃 생성/제거. 레이아웃은 맵마다 다시 생성됨
 * 화면 Push / Remove 진입점
 * 레이아웃이 있는 동안 UI 공통 IMC 적용
 * 게임 전체 수명의 ViewModel을 생성해서 MVVM 전역 Viewmodel Collection에 등록 / 해제 (VM 보관은 Collection)
 * 화면 페이드 메시지를 받아 페이드 중에만 Ticker로 레이아웃 불투명도를 갱신 (카메라 페이드는 UI를 덮지 않음)
 * Message.UI.OpenScreen을 받아 UCXUISettings::Screens에 지정된 화면을 열기. 이미 스택에 있으면 새로 만들지 않고 위로 올림
 * 
 * *화면이 스스로 닫힐 때는 RemoveWidget 보다는 개별 위젯의 DeactivateWidget()을 호출(Stack이 자동 제거)
 */
UCLASS()
class CXVI_API UCXUIManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	// 싱글 플레이 기준: 첫 번째 로컬 플레이어의 UI Manager
	static UCXUIManagerSubsystem& Get(const UObject* WorldContextObject);

	static UCXUIManagerSubsystem* TryGet(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

public:
	// Layout
	void CreatePrimaryLayout(APlayerController* InPlayerController);

	void DestroyPrimaryLayout();

	UCXPrimaryGameLayout* GetPrimaryLayout() const { return PrimaryLayout; }

public:
	// Widget
	template <typename ActivatableWidgetT = UCommonActivatableWidget>
	ActivatableWidgetT* PushWidgetToLayer(const FGameplayTag& InLayerTag, TSubclassOf<ActivatableWidgetT> InWidgetClass)
	{
		return PrimaryLayout ? PrimaryLayout->PushWidgetToLayerStack<ActivatableWidgetT>(InLayerTag, InWidgetClass) : nullptr;
	}

	template <typename ActivatableWidgetT = UCommonActivatableWidget>
	ActivatableWidgetT* PushWidgetToLayer(const FGameplayTag& InLayerTag, TSubclassOf<ActivatableWidgetT> InWidgetClass, TFunctionRef<void(ActivatableWidgetT&)> InInitFunc)
	{
		return PrimaryLayout ? PrimaryLayout->PushWidgetToLayerStack<ActivatableWidgetT>(InLayerTag, InWidgetClass, InInitFunc) : nullptr;
	}

	UFUNCTION(BlueprintCallable, Category = "CXVI|UI", meta = (DisplayName = "Push Widget To Layer", DeterminesOutputType = "InWidgetClass"))
	UCommonActivatableWidget* K2_PushWidgetToLayer(UPARAM(meta = (Categories = "UI.Layer")) FGameplayTag InLayerTag, TSubclassOf<UCommonActivatableWidget> InWidgetClass);

	// 외부에서 강제로 위젯을 닫을 때 호출
	UFUNCTION(BlueprintCallable, Category = "CXVI|UI")
	void RemoveWidget(UCommonActivatableWidget* InWidget);

private:
	// Add : bInApply == true
	// Remove : bInApply == false
	void ApplyUIInputMapping(bool bInApply) const;
	
	// Apply : 기본 키 이외에 WASD을 내비게이션 등록 / Restore : 등록 전 설정으로 복구
	void ApplyNavigationConfig(bool bInApply);

	// 새 레벨 시작 시 이전 레벨 화면이 남긴 입력 설정을 버리고 게임 기본 입력으로 초기화
	// (CommonUI는 활성 화면이 없어져도 입력 설정을 되돌리지 않고, Action Router는 레벨이 바뀌어도 유지됨)
	void ResetInputConfigToGameDefault() const;

private:
	// ViewModel

	// 전역 관리 ViewModel 생성 후 MVVM 전역 Viewmodel Collection에 등록 / Collection에서 찾아 정리 후 등록 해제
	void CreateGlobalViewModel();

	void DestroyGlobalViewModel();

private:
	// Open Screen
	void HandleOpenScreen(const FGameplayTag& InChannel, const FCXOpenScreenMessage& InMessage);

private:
	// Screen Fade
	void HandleScreenFade(const FGameplayTag& InChannel, const FCXScreenFadeMessage& InMessage);

	// return : true = 다음 프레임에도 호출, false = 페이드 종료 (Ticker 자동 해제)
	bool TickScreenFade(float InDeltaTime);

	void StopScreenFadeTicker();

	void ApplyScreenFadeToLayout() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCXPrimaryGameLayout> PrimaryLayout;

	// 복구할 Slate 내비게이션 캐시
	TSharedPtr<FNavigationConfig> PreviousNavigationConfig;

	FCXGameplayMessageListenerHandle ScreenFadeListener;

	FCXGameplayMessageListenerHandle OpenScreenListener;

	// 페이드 중에만 유효
	FTSTicker::FDelegateHandle ScreenFadeTickerHandle;

	float ScreenFadeFromAlpha = 0.f;

	float ScreenFadeToAlpha = 0.f;

	float ScreenFadeDuration = 0.f;

	float ScreenFadeElapsed = 0.f;

	// 현재 화면 가림 정도 (0 ~ 1). 레이아웃은 맵마다 다시 생성되므로 여기에 보관
	float ScreenFadeAlpha = 0.f;

#if WITH_EDITOR
private:
	// 에디터 PIE 전용
	// 화면 활성화 시점에 뷰포트 포커스가 없으면 CommonUI가 입력 설정을 건너뛰므로,
	// 뷰포트에 포커스가 들어올 때 맨 위 화면의 입력 설정과 포커스를 다시 적용
	void HandleSlateFocusChanging(const FFocusEvent& InFocusEvent, const FWeakWidgetPath& InOldFocusedPath, const TSharedPtr<SWidget>& InOldFocusedWidget, const FWidgetPath& InNewFocusedPath, const TSharedPtr<SWidget>& InNewFocusedWidget);

	void ReapplyTopWidgetInputConfig();

	FDelegateHandle FocusChangingHandle;
#endif
};

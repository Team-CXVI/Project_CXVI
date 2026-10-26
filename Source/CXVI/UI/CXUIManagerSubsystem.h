// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "CXPrimaryGameLayout.h"
#include "GameplayTagContainer.h"

#include "CXUIManagerSubsystem.generated.h"

class APlayerController;
struct FCXNavigationConfig;

DECLARE_LOG_CATEGORY_EXTERN(LogCXUI, Log, All);


/**
 * UI 매니저 (로컬 플레이어마다 하나)
 * - 기능
 * 루트 레이아웃 생성/제거. 레이아웃은 맵마다 다시 생성됨
 * 화면 Push / Remove 진입점
 * 레이아웃이 있는 동안 UI 공통 IMC 적용
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

private:
	UPROPERTY(Transient)
	TObjectPtr<UCXPrimaryGameLayout> PrimaryLayout;

	// 복구할 Slate 내비게이션 캐시
	TSharedPtr<FNavigationConfig> PreviousNavigationConfig;


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

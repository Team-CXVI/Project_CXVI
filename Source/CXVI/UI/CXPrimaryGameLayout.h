// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include <CommonActivatableWidget.h>
#include <Widgets/CommonActivatableWidgetContainer.h>
#include <GameplayTagContainer.h>

#include "CXPrimaryGameLayout.generated.h"

/**
 * 루트 레이아웃
 * - 레이어 태그(UI.Layer.*)로 레이어 관리 + 각 레이어는 위젯 컨테이너 베이스 (UCommonActivatableWidgetStack / UCommonActivatableWidgetQueue)
 * - 레이어 등록은 BP(WBP_PrimaryGameLayout)의 OnInitialized에서 RegisterLayer로 수행
 * - Layout 자신은 UCommonActivatableWidget이 아니므로 입력을 받지 않음
 */
UCLASS(Abstract, meta = (DisableNativeTick))
class CXVI_API UCXPrimaryGameLayout : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	UCommonActivatableWidgetContainerBase* GetLayerWidget(const FGameplayTag& InLayerTag) const;

	template <typename ActivatableWidgetT = UCommonActivatableWidget>
	ActivatableWidgetT* PushWidgetToLayerStack(const FGameplayTag& InLayerTag, UClass* InWidgetClass)
	{
		return PushWidgetToLayerStack<ActivatableWidgetT>(InLayerTag, InWidgetClass, [](ActivatableWidgetT&) {});
	}

	// InInitFunc : 위젯이 활성화 되기 전에 데이터를 넣을 초기화 람다 (팝업에 띄울 문구 등 동적으로 초기화 데이터를 밀어넣어야 하는 경우)
	template <typename ActivatableWidgetT = UCommonActivatableWidget>
	ActivatableWidgetT* PushWidgetToLayerStack(const FGameplayTag& InLayerTag, UClass* InWidgetClass, TFunctionRef<void(ActivatableWidgetT&)> InInitFunc)
	{
		static_assert(TIsDerivedFrom<ActivatableWidgetT, UCommonActivatableWidget>::IsDerived, "Only CommonActivatableWidget can be used here");

		if (nullptr == InWidgetClass)
		{
			return nullptr;
		}

		UCommonActivatableWidgetContainerBase* Layer = GetLayerWidget(InLayerTag);
		if (nullptr == Layer)
		{
			return nullptr;
		}

		return Layer->AddWidget<ActivatableWidgetT>(InWidgetClass, InInitFunc);
	}

	// LayerTag를 모르는 상태에서 UClass로 접근해서 위젯을 삭제
	void FindAndRemoveWidgetFromLayer(UCommonActivatableWidget* InWidget);

protected:
	UFUNCTION(BlueprintCallable, Category = "CXVI|UI")
	void RegisterLayer(UPARAM(meta = (Categories = "UI.Layer")) FGameplayTag InLayerTag, UCommonActivatableWidgetContainerBase* InLayerWidget);

private:
	// 각 레이어 별 위젯 스택 관리 컨테이너
	UPROPERTY(Transient, meta = (Categories = "UI.Layer"))
	TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>> Layers;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"

#include "CXCombatantEntryWidget.generated.h"

class UImage;

/**
 * 파티 패널 한 칸 (WBP_CombatantEntry의 부모)
 * - 파티 패널의 Viewmodel Extension(Set Items)이 생성하고 Entry Viewmodel(CXCombatantViewModel)을 연결
 * - HP 바 / 숫자는 WBP의 View Bindings로 직접 바인딩
 * - AP 칸은 개수가 바뀌므로 C++에서 생성 : WBP에서 AP → SetCurrentAP, MaxAP → SetMaxAP 바인딩
 * - AP 1칸 = Mana 1
 */
UCLASS(Abstract)
class CXVI_API UCXCombatantEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "CXVI|Combat")
	void SetCurrentAP(float InValue);

	UFUNCTION(BlueprintCallable, Category = "CXVI|Combat")
	void SetMaxAP(float InValue);

private:
	void RefreshAPSlots();
	
private:
	// AP 칸을 담을 패널 (HorizontalBox 등)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> APSlotPanel;

	UPROPERTY(EditAnywhere, Category = "AP")
	FSlateBrush FilledSlotBrush;

	UPROPERTY(EditAnywhere, Category = "AP")
	FSlateBrush EmptySlotBrush;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> APSlotImages;

	int32 CurrentAP = 0;

	int32 MaxAP = 0;
	
};

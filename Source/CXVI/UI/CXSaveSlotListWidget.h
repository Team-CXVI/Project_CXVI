// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/CXActivatableWidget.h"

#include "CXSaveSlotListWidget.generated.h"

class UCXSaveSlotEntryWidget;
class UPanelWidget;

/**
 * 저장 슬롯 목록 (WBP_SaveSlotList의 부모)
 * - 불러오기 화면. 타이틀의 이어하기가 UI.Layer.Menu에 Push
 * - 입력 모드 Menu, 뒤로가기로 닫힘
 * - 자동 저장 슬롯 + 수동 저장 슬롯(MaxManualSaveSlots)만큼 항목을 한 번만 생성하고 활성화될 때마다 표시 갱신
 *   (Stack이 위젯을 재사용하므로 저장 내용은 활성화 시점에 다시 읽음)
 * - 저장 파일이 없는 슬롯은 비활성화 (내비게이션에서 제외)
 * - 첫 포커스는 활성화된 첫 항목. 항목이 없을 때만 DefaultFocusWidgetName 사용
 */
UCLASS(Abstract)
class CXVI_API UCXSaveSlotListWidget : public UCXActivatableWidget
{
	GENERATED_BODY()
	
public:
	UCXSaveSlotListWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativeOnInitialized() override;

	virtual void NativeOnActivated() override;

	virtual UWidget* NativeGetDesiredFocusTarget() const override;

private:
	void HandleSlotClicked(int32 InSlotIndex);

protected:
	// 슬롯 항목 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Save Slot")
	TSubclassOf<UCXSaveSlotEntryWidget> SlotEntryClass;

	// 슬롯 항목을 넣을 패널
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> SlotEntryBox;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCXSaveSlotEntryWidget>> SlotEntries;
	
};

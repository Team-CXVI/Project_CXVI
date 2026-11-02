// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"

#include "CXSaveSlotMemberWidget.generated.h"

class UCommonTextBlock;

/**
 * 저장 슬롯의 파티원 한 명 (WBP_SaveSlotMember의 부모)
 * - 슬롯 항목(UCXSaveSlotEntryWidget)이 저장된 파티원 수만큼 생성
 * - 캐릭터 이름과 캐릭터 레벨 표시
 */
UCLASS(Abstract)
class CXVI_API UCXSaveSlotMemberWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	void SetMember(const FText& InName, int32 InLevel);

protected:
	// 캐릭터 레벨 표시 형식. {Level} = 레벨
	UPROPERTY(EditDefaultsOnly, Category = "Save Slot")
	FText LevelFormat;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> NameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> LevelText;
	
};

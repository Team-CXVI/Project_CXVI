// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/CXButtonBase.h"

#include "CXSaveSlotEntryWidget.generated.h"

class UCommonTextBlock;
class UCXSaveSlotMemberWidget;
class UImage;
class UPanelWidget;
struct FCXPartyMemberState;
struct FCXSaveHeader;

/**
 * 저장 슬롯 한 칸 (WBP_SaveSlotEntry의 부모)
 * - 슬롯 목록(UCXSaveSlotListWidget)이 슬롯마다 하나씩 생성
 * - 슬롯 이름은 베이스의 ButtonTextBlock에 표시 (버튼 상태에 맞는 텍스트 스타일 적용)
 * - 저장 파일이 있으면 저장 시각 / 플레이 시간 / 맵 레벨 이름 / 맵 레벨 대표 이미지 / 파티원(캐릭터 이름, 캐릭터 레벨)을 표시
 * - 저장 파일이 없으면 EmptySlotText만 표시
 */
UCLASS(Abstract)
class CXVI_API UCXSaveSlotEntryWidget : public UCXButtonBase
{
	GENERATED_BODY()
	
public:
	// 표시할 슬롯 지정. 슬롯 이름까지 설정
	void InitSlot(int32 InSlotIndex);

	// 저장 파일을 읽어 표시 갱신
	// return : 저장 파일이 있으면 true
	bool RefreshSlot();

private:
	void ShowEmptySlot();

	void RefreshLevel(const FCXSaveHeader& InHeader);

	void RefreshParty(const TArray<FCXPartyMemberState>& InParty);
	
protected:
	// 자동 저장 슬롯 이름
	UPROPERTY(EditDefaultsOnly, Category = "Save Slot")
	FText AutoSlotName;

	// 수동 저장 슬롯 이름. {Index} = 슬롯 번호
	UPROPERTY(EditDefaultsOnly, Category = "Save Slot")
	FText ManualSlotNameFormat;

	// 저장 파일이 없을 때 저장 시각 자리에 표시
	UPROPERTY(EditDefaultsOnly, Category = "Save Slot")
	FText EmptySlotText;

	// 파티원 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Save Slot")
	TSubclassOf<UCXSaveSlotMemberWidget> MemberClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> SaveTimeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> PlayTimeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> LevelNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> LevelImage;

	// 파티원 위젯을 넣을 패널
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> PartyMemberBox;

private:
	int32 SlotIndex = INDEX_NONE;
};

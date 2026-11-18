// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"

#include "CXCombatVictoryMemberWidget.generated.h"

class UImage;
class UTexture2D;
class UCommonTextBlock;
class UProgressBar;

/**
 * 전투 승리 화면의 파티원 결과 한 명 (WBP_CombatVictoryMember의 부모)
 * - 승리 화면(UCXCombatVictoryWidget)이 파티원별 결과 수만큼 생성
 * - 초상화, 현재 레벨(경험치 반영 후), HP 바(전투 HP + 레벨업 회복 반영 후) 표시
 * - 레벨이 올랐을 때만 LevelUpMark 표시
 * - 초상화는 Soft 참조라 SetBrushFromSoftTexture로 비동기 로드
 */
UCLASS(Abstract)
class CXVI_API UCXCombatVictoryMemberWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// InHealthPercent : 0 ~ 1
	void SetMember(TSoftObjectPtr<UTexture2D> InPortrait, int32 InLevel, bool bInLeveledUp, float InHealthPercent);

protected:
	// 현재 레벨 표시 형식. {Level} = 레벨
	UPROPERTY(EditDefaultsOnly, Category = "Result")
	FText LevelFormat;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> PortraitImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> LevelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HPBar;

	// 레벨업 표시 (문구 / 이미지 등 종류는 WBP에서 결정)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> LevelUpMark;
};

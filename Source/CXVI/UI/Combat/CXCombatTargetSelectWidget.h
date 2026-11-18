// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"

#include "CXCombatTargetSelectWidget.generated.h"

class ACXCombatCharacterPlayer;
class UTextBlock;
struct FCXActionSelection;
struct FCXCombatCommandMenuMessage;

/**
 * 전투 타겟 선택 UI (WBP_CombatTargetSelect의 부모, WBP_CombatHUD에 배치)
 * - Message.Combat.CommandMenuChanged를 받아 MenuState가 TargetSelect일 때만 표시
 * - 발동할 행동의 이름과 설명을 채움. 취소 키 안내는 WBP에 배치한 WBP_InputHint가 담당
 * - 입력은 처리하지 않음. 타겟 조작은 ACXCommandCharacter가 담당
 * - 구독 기간은 Construct ~ Destruct. HUD Push가 전투 시작(BeginBattle)보다 먼저라 첫 턴부터 받을 수 있음
 */
UCLASS(Abstract)
class CXVI_API UCXCombatTargetSelectWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

private:
	void HandleCommandMenuChanged(const FGameplayTag& InChannel, const FCXCombatCommandMenuMessage& InMessage);

	void RefreshActionText(const FCXActionSelection& InSelection, const ACXCombatCharacterPlayer* InCommandCharacter);

private:
	// 기본 공격은 스킬 테이블에 행이 없어 WBP Class Defaults에서 지정
	UPROPERTY(EditDefaultsOnly, Category = "TargetSelect")
	FText BasicAttackName;

	UPROPERTY(EditDefaultsOnly, Category = "TargetSelect", meta = (MultiLine = "true"))
	FText BasicAttackDescription;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ActionNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;

	FCXGameplayMessageListenerHandle CommandMenuListener;
	
};

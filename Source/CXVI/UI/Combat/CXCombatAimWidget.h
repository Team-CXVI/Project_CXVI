// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"

#include "CXCombatAimWidget.generated.h"

struct FCXCombatCommandMenuMessage;

/**
 * 전투 조준 사격 UI (WBP_CombatAim의 부모, WBP_CombatHUD에 배치)
 * - Message.Combat.CommandMenuChanged를 받아 조준 모드일 때만 표시
 * - 크로스헤어와 뒤로 키 안내(WBP_InputHint)는 WBP에 배치
 * - 입력은 처리하지 않음. 조준 조작은 ACXCommandCharacter가 담당
 * - 구독 기간은 Construct ~ Destruct. HUD Push가 전투 시작(BeginBattle)보다 먼저라 첫 턴부터 받을 수 있음
 */
UCLASS(Abstract)
class CXVI_API UCXCombatAimWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

private:
	void HandleCommandMenuChanged(const FGameplayTag& InChannel, const FCXCombatCommandMenuMessage& InMessage);

private:
	FCXGameplayMessageListenerHandle CommandMenuListener;
	
	
};

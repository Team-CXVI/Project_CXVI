// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"

#include "CXCombatActionHintWidget.generated.h"

class UCXHoldActionWidget;
struct FCXCombatCommandAvailabilityMessage;

/**
 * 전투 HUD의 키 힌트 패널 (WBP_CombatActionHint의 부모, WBP_CombatHUD에 배치)
 * - Message.Combat.CommandAvailabilityChanged를 받아 칸들을 활성화 / 비활성화
 *   비활성 칸은 입력을 받지 않고 화면에서 사라짐 (UCXHoldActionWidget::SetHoldActionActive)
 * - 구독 기간은 Construct ~ Destruct. HUD Push가 전투 시작(BeginBattle)보다 먼저라 첫 턴부터 받을 수 있음
 */
UCLASS(Abstract)
class CXVI_API UCXCombatActionHintWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

private:
	void HandleCommandAvailabilityChanged(const FGameplayTag& InChannel, const FCXCombatCommandAvailabilityMessage& InMessage);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXHoldActionWidget> SkipTurnHold;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXHoldActionWidget> FleeHold;

	FCXGameplayMessageListenerHandle CommandAvailabilityListener;
};

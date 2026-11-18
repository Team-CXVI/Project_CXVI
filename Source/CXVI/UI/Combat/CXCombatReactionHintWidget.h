// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"

#include "CXCombatReactionHintWidget.generated.h"

class UCXInputHintWidget;
struct FCXCombatReactionAvailabilityMessage;

/**
 * 몬스터 공격 중 회피 / 패링 키 안내 (WBP_CombatReactionHint의 부모, WBP_CombatHUD에 배치)
 * - Message.Combat.ReactionAvailabilityChanged를 받아 회피 칸과 패링 칸을 각각 표시 / 숨김
 *   표시 구간은 몬스터 행동 중(행동 시작 / 접근 포함 ~ 공격자의 행동 종료)
 *   전체 공격처럼 패링할 수 없는 공격이면 패링 칸만 숨김
 * - 키 아이콘은 WBP에 배치한 WBP_InputHint(UCXInputHintWidget)가 담당
 * - 입력은 처리하지 않음. 회피 / 패링 조작은 ACXCommandCharacter가 담당
 * - 구독 기간은 Construct ~ Destruct. HUD Push가 전투 시작(BeginBattle)보다 먼저라 첫 턴부터 받을 수 있음
 */
UCLASS(Abstract)
class CXVI_API UCXCombatReactionHintWidget : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

private:
	void HandleReactionAvailabilityChanged(const FGameplayTag& InChannel, const FCXCombatReactionAvailabilityMessage& InMessage);

	static void SetHintVisible(UCXInputHintWidget* InHint, bool bInVisible);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXInputHintWidget> DodgeHint;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXInputHintWidget> ParryHint;

	FCXGameplayMessageListenerHandle ReactionAvailabilityListener;
};

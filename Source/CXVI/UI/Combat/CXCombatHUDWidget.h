// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/CXActivatableWidget.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"
#include "Input/UIActionBindingHandle.h"

#include "CXCombatHUDWidget.generated.h"

class UCXCombatViewModel;
class UCXFieldMenuWidget;
class UBorder;
class UTextBlock;
class UWidgetAnimation;
class UInputAction;
struct FCXCombatActionStartedMessage;
struct FCXCombatCommandMenuMessage;

/**
 * 전투 HUD 베이스 (WBP_CombatHUD의 부모)
 * - 전투 Scene 정의의 HUDClass로 지정 → Scene이 UI.Layer.Game에 Push
 * - 입력 모드 GameAndMenu (전투 입력 + UI 입력)
 * - SetCombatViewModel : WBP에 선언한 CXCombatViewModel Viewmodel(생성 방식 Manual)에 주입
 * - 파티 패널 / 보스 HP UI 배치와 바인딩은 WBP의 View Bindings에서
 * - 시스템 메시지 : Message.Combat.ActionStarted를 받아 잠깐 표시 후 숨김
 *   - 지금은 스킬(ActionType = Skill, 이름 있음)일 때만 스킬 이름 표시. 플레이어 / 몬스터 공통
 *   - 표시 / 숨김은 프레임(SystemMessageBox) 단위. 문구는 그 안의 SystemMessageText
 *   - 표시 시간 / 연출은 SystemMessageAnim이 담당. 끝나면 SystemMessageBox를 숨김
 *   - 표시 중 새 메시지가 오면 문구를 바꾸고 애니메이션을 처음부터 다시 재생
 *   - 구독 기간은 Construct ~ Destruct (HUD Push가 전투 시작보다 먼저라 첫 턴부터 받음)
 * - 메뉴 열기 : OpenMenuAction을 Game 모드 UI 액션으로 바인딩 → UI.Layer.GameMenu에 MenuClass Push
 *   - ESC가 뒤로가기(IA_CB_Cancel)로 쓰이는 상태(스킬 / 아이템 메뉴, 대상 선택)에서는 바인딩을 해제
 *     CommonUI가 처리한 키는 Player 입력까지 가지 않으므로, 해제해야 커맨드 캐릭터가 ESC를 받음
 *   - 조준 모드에서도 바인딩을 해제 (ESC로 메뉴를 열지 않음)
 *   - 상태는 Message.Combat.CommandMenuChanged로 받고, 뒤로가기 상태인지는 이 위젯이 판단
 */
UCLASS(Abstract)
class CXVI_API UCXCombatHUDWidget : public UCXActivatableWidget
{
	GENERATED_BODY()


public:
	UCXCombatHUDWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void SetCombatViewModel(UCXCombatViewModel* InViewModel);

protected:
	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* InAnimation) override;

private:
	void HandleActionStarted(const FGameplayTag& InChannel, const FCXCombatActionStartedMessage& InMessage);

	void ShowSystemMessage(const FText& InText);

	void HideSystemMessage();

	// 커맨드 메뉴 상태가 바뀌면 메뉴 열기 바인딩을 등록 / 해제 (Message.Combat.CommandMenuChanged)
	void HandleCommandMenuChanged(const FGameplayTag& InChannel, const FCXCombatCommandMenuMessage& InMessage);

	// true면 메뉴 열기 바인딩 등록, false면 해제. 같은 상태면 무시
	void SetOpenMenuBindingActive(bool bInActive);

	void HandleOpenMenu();

protected:
	// 메뉴 열기 입력 (IA_UI_OpenMenu. 키는 IMC_UI에 매핑)
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TObjectPtr<UInputAction> OpenMenuAction;

	// 메뉴 위젯 클래스 (WBP_FieldMenu)
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TSubclassOf<UCXFieldMenuWidget> MenuClass;

private:
	// 시스템 메시지 프레임 (SystemMessageText를 감쌈). 표시 / 숨김 대상
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SystemMessageBox;

	// 시스템 메시지 문구. 프레임 / 문구 중 하나라도 없으면 시스템 메시지를 표시하지 않음
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SystemMessageText;

	// 시스템 메시지 표시 연출 (나타남 → 유지 → 사라짐). 길이 = 표시 시간. SystemMessageBox에 걸 것
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> SystemMessageAnim;

	FCXGameplayMessageListenerHandle ActionStartedListener;

	FCXGameplayMessageListenerHandle CommandMenuListener;

	// 등록 중인 메뉴 열기 바인딩. 해제 상태면 무효
	FUIActionBindingHandle OpenMenuBinding;
};

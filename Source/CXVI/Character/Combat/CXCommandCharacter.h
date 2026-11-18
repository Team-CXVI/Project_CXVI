// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "CXVI/Tags/CXGameplayTags.h"
#include "CXVI/Foundation/Combat/CombatMessage/CXCombatMessageData.h"
#include "CXVI/Character/Combat/Data/CXCombatInputData.h"
#include "Subsystems/CXInputControlSubsystem.h"
#include "CXVI/Foundation/Combat/CXCombatTypes.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"

#include <InputActionValue.h>

#include "CXCommandCharacter.generated.h"



UCLASS()
class CXVI_API ACXCommandCharacter : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ACXCommandCharacter();

	void AddCommandCharacter(class ACXCombatCharacterPlayer* NewPlayer);

	// Setter
	void SetCombatInputMode(ECXCombatInputMode NewMode);
	void SetMenuState(ECommandMenuState NewState);
	void RefreshMenuCamera() const;
	// 커맨드 입력 가능 여부가 바뀌면 Message.Combat.CommandAvailabilityChanged 전송
	void NotifyCommandAvailability();



protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void UnPossessed() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void UpdateCommandCharacter();

	// 현재 입력 모드와 메뉴 상태에 맞는 입력 모드 태그. Disabled면 빈 태그
	FGameplayTag FindInputModeTagForCurrentState() const;

	// 입력 모드 태그를 현재 상태에 맞게 교체 (UCXInputControlSubsystem)
	void RefreshInputModeTag();

	// 키 입력 함수
private:
	void HandleInputConfirm();
	void HandleInputCancel();
	void HandleInputSlot1();
	void HandleInputSlot2();
	void HandleInputSlot3();
	void HandleInputSelectLeft();
	void HandleInputSelectRight();
	void HandleInputParry();
	void HandleInputDodge();
	void HandleInputFire();
	void HandleInputToggleAimMode();
	void HandleInputLook(const FInputActionValue& Value);

	void HandleInputFlee();
	void HandleInputSkipTurn();

	void HandleInputJustTiming();

	// UI의 커맨드 실행 요청 (Message.Combat.CommandRequest)
	void HandleCommandRequest(const FGameplayTag& InChannel, const FCXCombatCommandRequestMessage& InMessage);

	// 메뉴 표시 상태가 바뀌면 Message.Combat.CommandMenuChanged 전송
	void NotifyCommandMenu();

	void SubmitActionToPlayer(const FCXActionSelection& Selection);

	// 아이템 메뉴에서 슬롯 선택. ItemId는 ACXCombatCharacterPlayer::GetItemIdForSlot 기준. 빈 슬롯이면 무시
	void SelectItemSlot(int32 SlotIndex);

	void OnAimModeInput();

	void OnMouseLookInput(const FInputActionValue& Value);

	void OnFireInput();

	void HandleTargetSelectionRequested(
		ACXCombatCharacterPlayer* Sender,
		const TArray<TWeakObjectPtr<APawn>>& Candidates);

	void MoveTargetSelection(int32 Direction);
	void CancelTargetSelection();

protected:


private:
	// IMC 등록 / 해제는 전투 Scene(DA_Scene_Combat의 IMCs)이 담당. 이 Pawn은 입력 모드 태그만 교체
	UPROPERTY(EditDefaultsOnly, Category = "Input|MyTurn")
	TObjectPtr<class UInputAction> ConfirmAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|MyTurn")
	TObjectPtr<class UInputAction> CancelAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|MyTurn")
	TObjectPtr<class UInputAction> Slot1Action;

	UPROPERTY(EditDefaultsOnly, Category = "Input|MyTurn")
	TObjectPtr<class UInputAction> Slot2Action;

	UPROPERTY(EditDefaultsOnly, Category = "Input|MyTurn")
	TObjectPtr<class UInputAction> Slot3Action;

	UPROPERTY(EditDefaultsOnly, Category = "Input|EnemyTurn")
	TObjectPtr<class UInputAction> ParryAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|EnemyTurn")
	TObjectPtr<class UInputAction> DodgeAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|MyTurn")
	TObjectPtr<class UInputAction> LeftSelectAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|MyTurn")
	TObjectPtr<class UInputAction> RightSelectAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|MyTurn")
	TObjectPtr<class UInputAction> ToggleAimModeAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|MyTurn")
	TObjectPtr<class UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|MyTurn")
	TObjectPtr<class UInputAction> LookAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input|EnemyTurn")
	TObjectPtr<class UInputAction> JustTimingAction;

private:
	// 현재 입력상태
	ECXCombatInputMode CombatInputMode = ECXCombatInputMode::Disabled;
	ECommandMenuState MenuState = ECommandMenuState::ActionMenu;
	ECommandMenuState TargetReturnMenu = ECommandMenuState::ActionMenu;

	FCXActionSelection SavedSelection;

	// 현재 행동하는 아군
	TWeakObjectPtr<class ACXCombatCharacterPlayer> CommandCharacter;

	TArray<TWeakObjectPtr<class ACXCombatCharacterPlayer>> CommandCharacters;


	TArray<TWeakObjectPtr<APawn>> TargetCandidates;
	int32 SelectedTargetIndex = INDEX_NONE;

	// 인풋 핸들
	//FCXInputModeHandle ActionModeHandle;
	// 현재 요청 중인 입력 모드 태그. 한 번에 하나만 요청
	FCXInputModeHandle InputModeHandle;
	FGameplayTag AppliedInputModeTag;

	FCXGameplayMessageListenerHandle CommandRequestListener;

	// 마지막으로 전송한 커맨드 입력 가능 여부
	bool bCommandAvailable = false;

	// 마지막으로 전송한 도망 가능 여부
	bool bFleeAvailable = false;

	// 마지막으로 전송한 Command Menu 상태
	bool bCommandMenuActive = false;
	bool bLastCommandMenuAimMode = false;
	ECommandMenuState LastCommandMenuState = ECommandMenuState::ActionMenu;
	TWeakObjectPtr<class ACXCombatCharacterPlayer> LastCommandMenuCharacter;
};

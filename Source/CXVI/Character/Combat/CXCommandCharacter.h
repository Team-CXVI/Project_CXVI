// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "CXVI/Tags/CXGameplayTags.h"
#include "CXVI/Foundation/Combat/CombatMessage/CXCombatMessageData.h"
#include "CXVI/Character/Combat/Data/CXCombatInputData.h"
#include "Subsystems/CXInputControlSubsystem.h"

#include <InputActionValue.h>

#include "CXCommandCharacter.generated.h"

UENUM()
enum class ECommandMenuState :uint8
{
	ActionMenu,
	ItemMenu,
	SkillMenu,
	TargetSelect
};

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



protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void PossessedBy(AController* NewController) override;

	virtual void UnPossessed() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void UpdateCommandCharacter();
	void RemoveInputMappingContexts();

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

	void HandleInputRunaway();
	void HandleInputSkipTurn();

	void SubmitActionToPlayer(const FCXActionSelection& Selection);

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
	UPROPERTY(EditDefaultsOnly, Category = "Input|MyTurn")
	TObjectPtr<class UInputMappingContext> ActionMappingContext;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input|EnemyTurn")
	TObjectPtr<class UInputMappingContext> ReactionMappingContext;

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

	UPROPERTY(EditDefaultsOnly, Category = "Input|MyTurn")
	TObjectPtr<class UInputAction> SkipTurnAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|MyTurn")
	TObjectPtr<class UInputAction> RunawayAction;

	TWeakObjectPtr<class UEnhancedInputLocalPlayerSubsystem> InputSubsystem;
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
	FCXInputModeHandle ReactionModeHandle;

};

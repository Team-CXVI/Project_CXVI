// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "CXVI/Foundation/Combat/CombatMessage/CXCombatMessageData.h"
#include "CXVI/Character/Combat/Data/CXCombatInputData.h"
#include <InputActionValue.h>

#include "CXCommandCharacter.generated.h"

UENUM()
enum class ECombatInputMode : uint8
{
	Disabled,
	Command,
	Reaction
};

UENUM()
enum class ECommandKey : uint8
{
	Q, W, E, F, A, D, ESC
};

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
	void SetCombatInputMode(ECombatInputMode NewMode);
	void SetMenuState(ECommandMenuState NewState);

private:
	void UpdateCommandCharacter();
	void RefreshInputMappingContexts();
	void RemoveInputMappingContexts();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void PossessedBy(AController* NewController) override;

	virtual void UnPossessed() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;


	// 키 입력 함수
private:
	void OnInputQ();
	void OnInputW();
	void OnInputE();
	void OnInputF();
	void OnInputA();
	void OnInputD();
	void OnInputESC();

	void HandleMenuInput(ECommandKey Key);

	void HandleActionMenuInput(ECommandKey Key);
	void HandleItemMenuInput(ECommandKey Key);
	void HandleSkillMenuInput(ECommandKey Key);
	void HandleTargetSelectInput(ECommandKey Key);

	void SubmitActionToPlayer(const FCXActionSelection& Selection);

	void HandleInputAction(const struct FInputActionInstance& Instance, ETriggerEvent TriggerEvent);
	void HandleInputCommand(const FInputActionValue& Value, ECXInputCommand Command);


	void OnBasicAttackInput();

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
	/*UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> AimModeAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> QKeyAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> WKeyAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> EKeyAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> FKeyAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> AKeyAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> DKeyAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> ESCKeyAction;*/

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UCXCombatInputData> CombatInputData;

	TWeakObjectPtr<class UEnhancedInputLocalPlayerSubsystem> InputSubsystem;
	TArray<TWeakObjectPtr<class UInputMappingContext>> ActiveMappingContexts;

private:
	// 현재 입력상태
	ECombatInputMode CombatInputMode = ECombatInputMode::Disabled;
	ECommandMenuState MenuState = ECommandMenuState::ActionMenu;
	ECommandMenuState TargetReturnMenu = ECommandMenuState::ActionMenu;

	FCXActionSelection SavedSelection;

	// 현재 행동하는 아군
	TWeakObjectPtr<class ACXCombatCharacterPlayer> CommandCharacter;

	TArray<TWeakObjectPtr<class ACXCombatCharacterPlayer>> CommandCharacters;

	TArray<TWeakObjectPtr<APawn>> TargetCandidates;
	int32 SelectedTargetIndex = INDEX_NONE;

};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include <InputActionValue.h>

#include "CXCommandCharacter.generated.h"

UENUM()
enum class ECombatInputMode : uint8
{
	Disabled,
	Command,
	Reaction
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
	void SetCombatInput(bool bBattle);
	FORCEINLINE void SetCombatInputMode(ECombatInputMode NewMode) { UpdateCommandCharacter(); CombatInputMode = NewMode; }

private:
	void UpdateCommandCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;


	// 키 입력 함수
private:
	void OnBasicAttackInput();

	void OnAimModeInput();

	void OnMouseLookInput(const FInputActionValue& Value);

	void OnFireInput();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> BasicAttackAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> AimModeAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputAction> FireAction;


private:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputMappingContext> FieldMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputMappingContext> BattleMappingContext;

private:
	// 현재 입력상태
	ECombatInputMode CombatInputMode = ECombatInputMode::Disabled;

	// 현재 행동하는 아군
	TWeakObjectPtr<class ACXCombatCharacterPlayer> CommandCharacter;

	TArray<TWeakObjectPtr<class ACXCombatCharacterPlayer>> CommandCharacters;

};

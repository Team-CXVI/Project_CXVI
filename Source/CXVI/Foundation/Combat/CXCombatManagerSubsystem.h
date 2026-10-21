// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CXCombatManagerSubsystem.generated.h"

//class ABattleArena;

/**
 * 
 */
UCLASS()
class CXVI_API UCXCombatManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	UCXCombatManagerSubsystem();

	void RequestStartBattle(TArray<TSubclassOf<class ACXCombatCharacterBase>> Players, TArray<TSubclassOf<class ACXCombatCharacterBase>> Enemies, class APlayerController* PlayerController);

	void RequestEndBattle();


	//FORCEINLINE ABattleArena* GetBattleArena() const { return BattleArena; }

protected:
	virtual void PostInitialize() override;
private:
	void HandlePlayerInputModeChanged(bool bAllowed);

private:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<APawn> GASCharacterPlayer;

	UPROPERTY(VisibleAnywhere, Category = "Manager")
	TObjectPtr<class UCXTurnManager> TurnManager;

	UPROPERTY()
	TObjectPtr<APawn> StoredFieldPawn;
	UPROPERTY()
	TObjectPtr<APlayerController> StoredPlayerController;
};

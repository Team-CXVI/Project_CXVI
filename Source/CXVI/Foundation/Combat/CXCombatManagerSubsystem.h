// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "CombatMessage/CXCombatMessageData.h"
#include "CXVI/Foundation/Combat/CXArenaManager.h"

#include "CXCombatManagerSubsystem.generated.h"

class ACXTestCombatArena;

class ACXCommandCharacter;
class ULevel;

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

	FORCEINLINE ACXTestCombatArena* GetCombatArena() const { return TestCombatArena.Get(); }

protected:
	virtual void PostInitialize() override;

private:
	void HandlePlayerInputModeChanged(ECXCombatInputMode NewInputMode);

	void HandleArenaReady(const TArray<TObjectPtr<class ACXCombatCharacterBase>>& Players, const TArray<TObjectPtr<class ACXCombatCharacterBase>>& Enemies);

	UFUNCTION()
	void HandleArenaLevelReady();

private:
	UPROPERTY(VisibleAnywhere, Category = "BattleArena")
	TWeakObjectPtr<class ACXCommandCharacter> WeakCommandCharacter;


	UPROPERTY(VisibleAnywhere, Category = "BattleArena")
	TWeakObjectPtr<ACXTestCombatArena> TestCombatArena;

	UPROPERTY(VisibleAnywhere, Category = "Manager")
	TObjectPtr<class UCXTurnManager> TurnManager;

	UPROPERTY(VisibleAnywhere, Category = "Manager")
	TObjectPtr<class UCXArenaManager> ArenaManager;

	FName CombatLevelName = FName(TEXT("CombatLevel"));

	bool bBattleRequested = false;
	bool bLoadingCombat = false;

	// 비동기 로드를 기다리는 동안 유지.
	UPROPERTY()
	FCXArenaSetupData PendingSetupData;


	UPROPERTY()
	TObjectPtr<APawn> StoredFieldPawn;
	UPROPERTY()
	TObjectPtr<APlayerController> StoredPlayerController;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CXArenaManager.generated.h"

class ACXCombatCharacterBase;
class ACXTestCombatArena;

USTRUCT()
struct FCXArenaSetupData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TSubclassOf<ACXCombatCharacterBase>> PlayerClasses;

	UPROPERTY()
	TArray<TSubclassOf<ACXCombatCharacterBase>> EnemyClasses;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FCXOnArenaReady, const TArray<TObjectPtr<ACXCombatCharacterBase>>&, const TArray<TObjectPtr<ACXCombatCharacterBase>>&);

/**
 * 
 */
UCLASS()
class CXVI_API UCXArenaManager : public UObject
{
	GENERATED_BODY()
	
public:
	virtual UWorld* GetWorld() const override;

	// 서브레벨 로드 표시 완료 후 호출.
	bool PrepareArena(const FCXArenaSetupData& SetupData, ACXTestCombatArena* InArena);

	void CleanupArena();
	
	FCXOnArenaReady OnArenaReady;

private:
	bool SpawnParty(const TArray<TSubclassOf<ACXCombatCharacterBase>>& Classes, bool bPlayerParty);

private:
	UPROPERTY()
	TWeakObjectPtr<ACXTestCombatArena> Arena;

	UPROPERTY()
	TArray<TObjectPtr<ACXCombatCharacterBase>> PlayerCharacters;

	UPROPERTY()
	TArray<TObjectPtr<ACXCombatCharacterBase>> EnemyCharacters;


};

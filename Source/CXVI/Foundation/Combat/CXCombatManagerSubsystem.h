// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

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
	void HandlePlayerInputModeChanged(bool bAllowed);

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

public:
	// TODO - Scene 구조로 변경
	// Scene 전환용 전투 시작 (UCXCombatScene::OnEnter에서 호출)
	// - 전투 레벨 로드/표시, 화면 페이드, 필드 Pawn 복귀는 Scene이 담당
	// - 넘겨받은 전투 레벨에서 아레나 / 커맨드 Pawn 검색 -> 전투원 생성 -> 턴 시작 (동기)
	// - 종료 시 Message.Combat.End 방송만 함. 정리는 CleanupSceneBattle (Scene Exit, 화면이 가려진 뒤)
	bool StartSceneCombat(const FCXArenaSetupData& InSetupData, const TArray<ULevel*>& InCombatLevels, APlayerController* InPlayerController);

	void CleanupSceneCombat();

private:
	void HandleSceneCombatEnded();

	// 전투 레벨에서 아레나 / 커맨드 Pawn 검색 (각각 정확히 1개)
	bool FindArenaActors(const TArray<ULevel*>& InCombatLevels, ACXTestCombatArena*& OutArena, ACXCommandCharacter*& OutCommandPawn) const;

private:
	// Scene 전환으로 시작한 전투 진행 중
	// 기존 bBattleRequested와 분리해서 RequestEndBattle(기존 경로)이 반응하지 않도록 함
	bool bSceneCombatActive = false;

	// Scene 전투 종료 핸들러 (기존 RequestEndBattle 바인딩은 유지해야 하므로 핸들로 해제)
	FDelegateHandle SceneCombatEndedHandle;
};

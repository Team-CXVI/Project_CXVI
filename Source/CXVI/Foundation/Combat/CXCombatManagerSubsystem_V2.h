// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "CXVI/Foundation/Combat/CXCombatTypes.h"

#include "CXCombatManagerSubsystem.generated.h"

class ACXCombatArena;
class ACXCommandCharacter;
class ACXCombatCharacterBase;
class UCXTurnManager;
class UCXSceneDefinition;

DECLARE_LOG_CATEGORY_EXTERN(LogCXCombat, Log, All);

/**
 * 전투 매니저
 * - 진입 창구 : RequestStartBattle에서 전투 가능 여부를 판단하고 현재 레벨의 전투 Scene Push를 요청
 * - 전투 Scene(UCXCombatScene)이 아레나 / 커맨드 Pawn / 전투원을 준비해서 BeginBattle로 넘기면 턴 진행
 * - 턴 매니저가 전투를 끝내면 Message.Combat.End 방송. 필드 복귀와 정리 시점은 전투 Scene이 결정 (OnExit -> EndBattle)
 * - 레벨 로드 / 표시, 빙의, 화면 전환은 Scene이 담당
 */
UCLASS()
class CXVI_API UCXCombatManagerSubsystem_V2 : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UCXCombatManagerSubsystem_V2& Get(const UObject* WorldContextObject);

	static UCXCombatManagerSubsystem_V2* TryGet(const UObject* WorldContextObject);

	// 전투 진입 요청. 조건이 맞으면 현재 레벨 정의의 전투 Scene을 Push (거절되면 false)
	// TODO : 파티 정보 출처가 정해지면 InEncounterId만 받도록 변경
	bool RequestStartBattle(FName InEncounterId, const TArray<TSubclassOf<ACXCombatCharacterBase>>& InPlayerClasses, const TArray<TSubclassOf<ACXCombatCharacterBase>>& InEnemyClasses);

	// 전투 Scene이 준비를 마친 뒤 호출 (Entering -> InBattle)
	bool BeginBattle(const FCXBattleSetup& InSetup);

	// 전투 Scene OnExit에서 호출. 턴 정지 + Idle (Entering 상태에서도 동작)
	void EndBattle();

	FORCEINLINE ECXCombatState GetCombatState() const { return CombatState; }

	// 커맨드 Pawn의 카메라 전환에서 사용
	FORCEINLINE ACXCombatArena* GetCombatArena() const { return CombatArena.Get(); }

protected:
	virtual void PostInitialize() override;

private:
	bool CanStartBattle(FName InEncounterId, const TArray<TSubclassOf<ACXCombatCharacterBase>>& InPlayerClasses, const TArray<TSubclassOf<ACXCombatCharacterBase>>& InEnemyClasses) const;

	// 현재 레벨 정의의 CombatScene (Scene 서브레벨은 현재 퍼시스턴트 레벨에 등록된 것만 로드 가능)
	UCXSceneDefinition* GetCombatSceneDefinition() const;

	void HandleBattleEnded(bool bInVictory);

	void HandlePlayerInputModeChanged(bool bAllowed);

private:
	UPROPERTY(VisibleAnywhere, Category = "Manager")
	TObjectPtr<UCXTurnManager> TurnManager;

	UPROPERTY(VisibleAnywhere, Category = "BattleArena")
	TWeakObjectPtr<ACXCombatArena> CombatArena;

	UPROPERTY(VisibleAnywhere, Category = "BattleArena")
	TWeakObjectPtr<ACXCommandCharacter> WeakCommandCharacter;

	ECXCombatState CombatState = ECXCombatState::Idle;
};

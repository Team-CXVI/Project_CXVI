// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "CXCombatTypes.generated.h"

class ACXCombatCharacterBase;
class ACXCombatArena;
class ACXCommandCharacter;

//DECLARE_LOG_CATEGORY_EXTERN(LogCXCombat, Log, All);

/**
 * 전투 진입 / 진행 / 종료 공용 타입
 * - 전투 Scene(Foundation/Scene)과 전투 매니저가 함께 참조하므로 전투 쪽 헤더에 둠 (의존 방향 : Scene -> Combat)
 */

 // 전투 진행 상태 (UCXCombatManagerSubsystem)
UENUM()
enum class ECXCombatState : uint8
{
	Idle,		// 전투 없음
	Entering,	// 전투 Scene Push 요청 ~ BeginBattle 전
	InBattle,	// 턴 진행 중
};

// 필드 -> 전투 (FCXSceneContext Payload)
USTRUCT(BlueprintType)
struct CXVI_API FCXCombatEnterParams
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FName EncounterId;

	// TODO : 임시. 파티 정보 출처가 정해지면 EncounterId 조회로 변경
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<TSubclassOf<ACXCombatCharacterBase>> PlayerClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<TSubclassOf<ACXCombatCharacterBase>> EnemyClasses;
};

// 전투 -> 필드 (PopScene Payload)
// TODO : 결과 내용이 정해지면 수정
USTRUCT(BlueprintType)
struct CXVI_API FCXCombatResult
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bVictory = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 EarnedExp = 0;
};

// 전투 Scene -> 전투 매니저 (전투 Scene에서 준비가 끝난 요소)
USTRUCT()
struct CXVI_API FCXBattleSetup
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<ACXCombatArena> Arena = nullptr;

	// 전투 Scene이 이미 빙의한 커맨드 Pawn
	UPROPERTY()
	TObjectPtr<ACXCommandCharacter> CommandPawn = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<ACXCombatCharacterBase>> Players;

	UPROPERTY()
	TArray<TObjectPtr<ACXCombatCharacterBase>> Enemies;
};

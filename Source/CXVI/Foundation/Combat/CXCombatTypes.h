// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "CXCombatTypes.generated.h"

class ACXCombatCharacterBase;
class ACXCombatArena;
class ACXCommandCharacter;
class UCXEnemyDefinition;

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

USTRUCT()
struct FCXInitialAttributes
{
	GENERATED_BODY()

	UPROPERTY()
	float MaxHealth = 200.f;

	UPROPERTY()
	float CurrentHealth = 200.f;

	UPROPERTY()
	float CombatSpeed = 200.f;

	UPROPERTY()
	float AttackPower = 40.f;

	UPROPERTY()
	float Defense = 40.f;

	UPROPERTY()
	float CriticalChance = 50.f;

	UPROPERTY()
	float CriticalDamageMultiplier = 2.f;

	UPROPERTY()
	float DodgeChance = 0.f;

};

USTRUCT(BlueprintType)
struct CXVI_API FCXCombatSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TSubclassOf<ACXCombatCharacterBase> CharacterClass;

	//// 스탯과 경험치의 출처. 클래스를 직접 지정 테스트에는 비어있을수 있다,.
	//UPROPERTY(BlueprintReadOnly, Category = "Combat")
	//TObjectPtr<UCXEnemyDefinition> Definition = nullptr;

	UPROPERTY()
	bool bHasInitialAttributes = false;

	UPROPERTY()
	FCXInitialAttributes InitialAttributes;
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
	TArray<FCXCombatSpawnEntry> PlayerClasses;

	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<TSubclassOf<ACXCombatCharacterBase>> EnemyClasses;*/
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TArray<FCXCombatSpawnEntry> EnemySpawns;
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

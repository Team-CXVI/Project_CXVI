// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/PrimaryAssetId.h"

#include "CXCombatTypes.generated.h"

class ACXCombatCharacterBase;
class ACXCombatArena;
class ACXCommandCharacter;
class UCXEnemyDefinition;
class UCXGameplayAbilityCombatBase;
class UTexture2D;

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
struct FCXMenuCameraPreset
{
	GENERATED_BODY()

	// 캐릭터 기준 카메라 위치. X는 앞, Y는 오른쪽, Z는 위.
	UPROPERTY(EditAnywhere, Category = "Camera")
	FVector CameraOffset = FVector(-450.f, 200.f, 120.f);

	// 플레이어 BP에 배치한 시선 기준점
	UPROPERTY(EditAnywhere, Category = "Camera")
	FName FocusName = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Camera")
	bool bUseOriginLocation = false;

	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "0.0", Units = "s"))
	float BlendDuration = 0.3f;
};

UENUM()
enum class ECommandMenuState :uint8
{
	ActionMenu,
	ItemMenu,
	SkillMenu,
	TargetSelect
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

USTRUCT()
struct FCXEnemySkillSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftClassPtr<UCXGameplayAbilityCombatBase> AbilityClass;

	UPROPERTY()
	int32 Weight = 1;

	// 스킬 행의 DisplayName. 행동 시작 메시지(Message.Combat.ActionStarted)에 사용
	UPROPERTY()
	FText DisplayName;
};

USTRUCT(BlueprintType)
struct CXVI_API FCXCombatSpawnEntry
{
	GENERATED_BODY()

	// 경로만 담음. 전투 Scene 전환의 LoadAssets 단계(화면이 가려진 뒤)에서 비동기 로드되고, 스폰 시 Get()으로 사용
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TSoftClassPtr<ACXCombatCharacterBase> CharacterClass;

	//// 스탯과 경험치의 출처. 클래스를 직접 지정 테스트에는 비어있을수 있다,.
	//UPROPERTY(BlueprintReadOnly, Category = "Combat")
	//TObjectPtr<UCXEnemyDefinition> Definition = nullptr;
	UPROPERTY()
	TArray<FCXEnemySkillSpawnEntry> EnemySkills;

	UPROPERTY()
	bool bHasInitialAttributes = false;

	UPROPERTY()
	FCXInitialAttributes InitialAttributes;

	UPROPERTY()
	FName SkillRowPrefix = NAME_None;

	// 적 전용. UCXEnemyDefinition에서 복사
	UPROPERTY()
	bool bIsBoss = false;

	// 적 전용. UCXEnemyDefinition에서 복사
	UPROPERTY()
	FText DisplayName;

	// UI 초상화. 파티원은 UCXCharacterDefinition, 적은 UCXEnemyDefinition에서 복사 (경로만 담음)
	UPROPERTY()
	TSoftObjectPtr<UTexture2D> Portrait;
};

// 필드 -> 전투 (FCXSceneContext Payload)
USTRUCT(BlueprintType)
struct CXVI_API FCXCombatEnterParams
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FName EncounterId;

	UPROPERTY(Transient)
	bool bIsPlayerAttack = false;

	// TODO : 임시. 파티 정보 출처가 정해지면 EncounterId 조회로 변경
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<FCXCombatSpawnEntry> PlayerClasses;

	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<TSubclassOf<ACXCombatCharacterBase>> EnemyClasses;*/
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TArray<FCXCombatSpawnEntry> EnemySpawns;
};

// 전투 승리 결과의 파티원 한 명분 (UserData 파티 순서)
// - 경험치를 반영하는 전투 매니저가 반영 전후 레벨을 기록
USTRUCT(BlueprintType)
struct CXVI_API FCXCombatMemberResult
{
	GENERATED_BODY()

public:
	bool IsLeveledUp() const { return NewLevel > OldLevel; }

public:
	// 캐릭터 정의 ID (UCXCharacterDefinition의 Asset Id)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FPrimaryAssetId CharacterId;

	// 경험치 반영 전 레벨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 OldLevel = 1;

	// 경험치 반영 후 레벨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 NewLevel = 1;

	// 전투 HP와 레벨업 회복을 반영한 뒤의 현재 HP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Health = 0.f;

	// 경험치 반영 후 레벨 기준 최대 HP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MaxHealth = 0.f;
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

	// 보스가 있는 전투에서 승리했으면 true (필드 복귀 시 클리어 화면 표시 기준)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bBossDefeated = false;

	// 승리 시 파티원별 결과. 승리가 아니면 비어 있음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<FCXCombatMemberResult> MemberResults;
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


	// 플레이어 선공 변수
	UPROPERTY()
	bool bIsPlayerAttack = false;
};

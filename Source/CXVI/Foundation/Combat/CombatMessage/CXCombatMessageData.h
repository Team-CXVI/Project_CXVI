#pragma once

#include "CoreMinimal.h"
#include "CXCombatMessageData.generated.h"

UENUM()
enum class ECXCombatPhase : uint8
{
	BeginPhase = 0,			// 턴시작
	BeforeSelectPhase,		// 전투 이외 데미지
	SelectActionPhase,		// 선택지
	SelectTargetPhase,		// 추가 선택지가 있다면
	ActionPhase,			// 행동
	ReturnPhase,			// 리액션
	EndPhase,				// 턴 끝
	PhaseCount
};

UENUM()
enum class ECXCombatMessageType : uint8
{
	None = 0,
	Damaged,				// 데미지 입었으니 죽었나 검사
	EnemyTargetsRequest,	// 반대팀 객체 배열을 보내                                                                                                                                                                         줘야함
	TeamTargetRequest,		// 우리팀 객체 배열 필요
	SelectionCancelled,		// 선택지에서 돌아가는 선택 이전페이즈로 가서 다시 뭐할지 골라야함
	SkippedTurn,			// 자기 턴을 스킵함.
	WantFlee,				// 도망
	Action,					// 행동
	TurnCompleted,
	AttackStarted,
	ParrySucceeded,
	ReturnCompleted,			
};

enum class ECXActionType : uint8
{
	None,
	BasicAttack,
	Skill,
	Item,
	CounterAttack,
};

enum class ECXTargetTeam : uint8
{
	Enemy,
	Player
};

UENUM()
enum class ECXCombatInputMode : uint8
{
	Disabled,
	Command,
	Reaction,
	JustTiming
};

USTRUCT()
struct FCXCombatMessageData
{
	GENERATED_BODY()

	ECXCombatPhase Phase = ECXCombatPhase::BeginPhase;
	ECXCombatMessageType MessageType = ECXCombatMessageType::None;

	// 현재 메시지를 보낸 캐릭터
	TWeakObjectPtr<APawn> Character;
	// 공격한 폰
	TWeakObjectPtr<APawn> Attacker;
	TArray<TWeakObjectPtr<APawn>> Targets;

	// 총 타격수
	int32 TotalHitCount = 0;

};

struct FCXActionSelection
{
	ECXActionType Type = ECXActionType::None;
	int32 SlotIndex = INDEX_NONE;
	ECXTargetTeam TargetTeam = ECXTargetTeam::Enemy;

	void Clear() { Type = ECXActionType::None; SlotIndex = INDEX_NONE; TargetTeam = ECXTargetTeam::Enemy; }
};

/** Message.Combat.End 채널 메시지 (전투 -> 전투 종료를 기다리는 쪽) */
USTRUCT(BlueprintType)
struct CXVI_API FCXCombatEndMessage
{
	GENERATED_BODY()

public:
	// TODO : OnBattleEnded에 결과가 추가되면 채움 (현재 항상 false)
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bVictory = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	double ElapsedSeconds = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 EarnedExp = 0;
};

/** Message.Combat.TurnOrderChanged 채널 메시지. 받는 쪽은 UCXCombatManagerSubsystem::GetUpcomingTurns()로 다시 읽음 */
USTRUCT(BlueprintType)
struct CXVI_API FCXTurnOrderChangedMessage
{
	GENERATED_BODY()
};

/** 전투 HUD에서 커맨드 캐릭터에 요청하는 행동 */
UENUM(BlueprintType)
enum class ECXCombatCommandRequest : uint8
{
	None,
	SkipTurn,	// 턴 넘기기
	Flee		// 도망
};

/** Message.Combat.CommandRequest 채널 메시지 (UI -> 커맨드 캐릭터) */
USTRUCT(BlueprintType)
struct CXVI_API FCXCombatCommandRequestMessage
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	ECXCombatCommandRequest Request = ECXCombatCommandRequest::None;
};

/** Message.Combat.CommandAvailabilityChanged 채널 메시지 (커맨드 캐릭터 -> UI). 값이 바뀔 때만 전송 */
USTRUCT(BlueprintType)
struct CXVI_API FCXCombatCommandAvailabilityMessage
{
	GENERATED_BODY()

public:
	// 내 턴의 행동 선택 메뉴일 때 true
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bAvailable = false;
};
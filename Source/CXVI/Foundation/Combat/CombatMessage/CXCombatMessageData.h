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
	ReactionPhase,			// 리액션
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
	Runaway,				// 도망
	Action,					// 행동
	TurnCompleted
};

enum class ECXActionType : uint8
{
	None,
	BasicAttack,
	Skill,
	Item
};

enum class ECXTargetTeam : uint8
{
	Enemy,
	Player
};



USTRUCT()
struct FCXCombatMessageData
{
	GENERATED_BODY()

	ECXCombatPhase Phase = ECXCombatPhase::BeginPhase;
	ECXCombatMessageType MessageType = ECXCombatMessageType::None;

	TWeakObjectPtr<APawn> Character;
	TArray<TWeakObjectPtr<APawn>> Targets;

};

struct FCXActionSelection
{
	ECXActionType Type = ECXActionType::None;
	int32 SlotIndex = INDEX_NONE;
	ECXTargetTeam TargetTeam = ECXTargetTeam::Enemy;

	void Clear() { Type = ECXActionType::None; SlotIndex = INDEX_NONE; TargetTeam = ECXTargetTeam::Enemy; }
};
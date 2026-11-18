#pragma once

#include "CoreMinimal.h"
#include "Foundation/Combat/CXCombatTypes.h"

#include "CXCombatMessageData.generated.h"

class ACXCombatCharacterPlayer;
class UCXJustTimingComponent;

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
	AimShotCompleted,
	ReturnedToOrigin,
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
	FName ItemId = NAME_None;

	void Clear() 
	{ 
		Type = ECXActionType::None; 
		SlotIndex = INDEX_NONE; 
		TargetTeam = ECXTargetTeam::Enemy; 
		ItemId = NAME_None;
	}
};

/** Message.Combat.End 채널 메시지 (전투 -> 전투 종료를 기다리는 쪽) */
USTRUCT(BlueprintType)
struct CXVI_API FCXCombatEndMessage
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bVictory = false;

	// 살아 있는 플레이어가 없음. bVictory와 bPartyWiped가 모두 false면 도망
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bPartyWiped = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	double ElapsedSeconds = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 EarnedExp = 0;

	// 승리 시 파티원별 결과 (경험치 반영 전후 레벨). 승리가 아니면 비어 있음
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TArray<FCXCombatMemberResult> MemberResults;
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

	// bAvailable이면서 이번 전투에서 도망이 가능할 때 true (보스전이면 false)
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bFleeAvailable = false;
};

/**
 * Message.Combat.CommandMenuChanged 채널 메시지 (커맨드 캐릭터 -> UI). 값이 바뀔 때만 전송
 * - 메뉴 상태만 전달. 표시 여부는 받는 쪽에서 판단
 */
USTRUCT()
struct CXVI_API FCXCombatCommandMenuMessage
{
	GENERATED_BODY()

public:
	// 내 턴에 커맨드 입력 중일 때 true. false면 아래 값은 현재 메뉴를 나타내지 않음
	UPROPERTY()
	bool bMenuActive = false;

	UPROPERTY()
	ECommandMenuState MenuState = ECommandMenuState::ActionMenu;

	UPROPERTY()
	bool bAimMode = false;

	UPROPERTY()
	TWeakObjectPtr<ACXCombatCharacterPlayer> CommandCharacter;

	// 현재 선택한 행동
	FCXActionSelection Selection;
};

/** 피해 처리 결과 종류 */
UENUM(BlueprintType)
enum class ECXDamageResult : uint8
{
	Hit,		// 피격
	Critical,	// 치명타 피격
	Dodged,		// 회피 성공 (피해 없음)
	Parried		// 패링 성공 (피해 없음)
};

/**
 * Message.Combat.DamageResult 채널 메시지 (전투 캐릭터 -> UI 등)
 * - 결과 사실만 전달. 표시 여부와 방법은 받는 쪽에서 판단
 * - 보내는 곳 : ACXCombatCharacterBase의 HandleDamageApplied / HandleDodgeSucceeded / HandleParrySucceeded
 */
USTRUCT(BlueprintType)
struct CXVI_API FCXDamageResultMessage
{
	GENERATED_BODY()

public:
	// 피해를 받은(회피 / 패링한) 캐릭터
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TWeakObjectPtr<AActor> Target;

	// 공격한 액터
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TWeakObjectPtr<AActor> Instigator;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	ECXDamageResult Result = ECXDamageResult::Hit;

	// 계산된 최종 피해 (OnDamageApplied와 같은 값). Dodged / Parried면 0
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float Amount = 0.f;
};

/** 저스트 타이밍 판정 구간 이벤트 종류 */
UENUM(BlueprintType)
enum class ECXJustTimingEvent : uint8
{
	Opened,		// 판정 구간 시작
	Succeeded,	// 성공 구간 안에서 입력
	Failed,		// 성공 구간 밖 입력 / 미입력으로 구간 종료 / 결과 없이 판정 기록 제거
	Cleared		// 결과 없이 취소 (어빌리티 시작 / 종료 / 중단 시 ResetTiming)
};

/**
 * Message.Combat.JustTiming 채널 메시지 (UCXJustTimingComponent -> UI 등)
 * - 구간 하나 = Source + HitNumber + TimingIndex
 * - Opened : SuccessStartRatio / SuccessEndRatio 사용
 * - Succeeded / Failed : Progress 사용 (결과가 정해진 순간의 진행도)
 * - 진행 중 진행도는 Source->GetWindowProgress로 조회
 */
USTRUCT(BlueprintType)
struct CXVI_API FCXJustTimingMessage
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	ECXJustTimingEvent Event = ECXJustTimingEvent::Opened;

	// 판정 구간을 가진 컴포넌트 (구간 구분 / 진행도 조회용)
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TWeakObjectPtr<UCXJustTimingComponent> Source;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 HitNumber = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 TimingIndex = INDEX_NONE;

	// 성공 구간 (0 ~ 1)
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float SuccessStartRatio = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float SuccessEndRatio = 1.f;

	// 결과가 정해진 순간의 진행도 (0 ~ 1)
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float Progress = 0.f;
};

/**
 * Message.Combat.ReactionAvailabilityChanged 채널 메시지 (전투 매니저 -> UI). 값이 바뀔 때만 전송
 * - 몬스터 행동 중(행동 시작 / 접근 포함 ~ 공격자의 행동 종료)일 때 방어 대상 기준의 가능 여부만 전달
 * - 표시 여부는 받는 쪽에서 판단
 */
USTRUCT(BlueprintType)
struct CXVI_API FCXCombatReactionAvailabilityMessage
{
	GENERATED_BODY()

public:
	// 살아 있는 방어 대상이 한 명 이상일 때 true
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bDodgeAvailable = false;

	// 그중 이번 공격을 패링할 수 있는 대상이 있을 때 true (전체 공격이면 false)
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bParryAvailable = false;
};

/**
 * Message.Combat.ActionStarted 채널 메시지 (전투 캐릭터 -> UI 등)
 * - 보내는 곳 : ACXCombatCharacterBase::ActionPhase (행동 요청 성공 시점 = 접근 이동 전)
 * - 행동 사실만 전달. 표시 여부와 문구는 받는 쪽에서 판단
 */
USTRUCT()
struct CXVI_API FCXCombatActionStartedMessage
{
	GENERATED_BODY()

public:
	// 행동한 캐릭터
	UPROPERTY()
	TWeakObjectPtr<AActor> Character;

	// 몬스터는 스킬 행으로 받은 어빌리티면 Skill, 아니면 BasicAttack
	ECXActionType ActionType = ECXActionType::None;

	// 스킬 행의 DisplayName (ST_Skill). 기본 공격 / 반격처럼 행이 없으면 비어 있음
	UPROPERTY()
	FText ActionName;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "CombatMessage/CXCombatMessageData.h"

#include "CXTurnManager.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerInputEnabledChanged, ECXCombatInputMode);
//DECLARE_MULTICAST_DELEGATE(FCXOnBattleEnded);
DECLARE_MULTICAST_DELEGATE_TwoParams(FCXOnBattleEnded, bool /*bVictory*/, double /*ElapsedSeconds*/);
DECLARE_MULTICAST_DELEGATE(FCXOnUpcomingTurnsChanged);

class ACXCombatCharacterBase;
struct FOnAttributeChangeData;

USTRUCT()
struct FTurnOrder
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<ACXCombatCharacterBase> Character = nullptr;

	// 다음 행동이 가능한 시간
	UPROPERTY()
	float NextActionTime = 0.f;

	// 우선순위 계산에 사용하는 속도
	UPROPERTY()
	float CombatSpeed = 1.f;

	// 시간과 속도가 같을 때 사용할 순서
	UPROPERTY()
	int32 RegistrationOrder = 0;
};


/**
 * 
 */
UCLASS()
class CXVI_API UCXTurnManager : public UObject
{
	GENERATED_BODY()
	
public:
	UCXTurnManager();

	// 배틀 시작 
	void StartBattle(
		TArray<TObjectPtr<ACXCombatCharacterBase>> Players, 
		TArray<TObjectPtr<ACXCombatCharacterBase>> Enemies
	);

	// 인풋모드 변경알림 델리게이트
	FOnPlayerInputEnabledChanged OnPlayerInputEnabledChanged;

	FCXOnBattleEnded OnBattleEnded;

	// 예측 턴 목록을 다시 계산했을 때 호출
	FCXOnUpcomingTurnsChanged OnUpcomingTurnsChanged;

	void StopBattle() { ResetBattle(); }

	// 현재 진행중인 전투의 경과 시간.
	double GetCombatElapsedSeconds() const;

	const TArray<TWeakObjectPtr<ACXCombatCharacterBase>>& GetUpcomingTurns() const { return UpcomingTurns; }

	void SetTurnPreviewCount(int32 NewCount);

private:

	// 초기화 함수.
	void ResetBattle();

	// 순서 정렬 - 우선순위 큐로 이동
	//void SortTurnOrder();
	// 다음 플레이어 찾기
	void FindNextPlayer();
	// 찾은 플레이어 실행
	void StartTurn();
	// 턴 완료
	void CompleteTurn();

	// 캐릭터의 메시지를 받아 처리하고 다음 페이즈 요청
	void HandleTurnResponse(const FCXCombatMessageData& Response);
	// 메시지 처리 함수.
	bool ProcessTurnResponse(const FCXCombatMessageData& Response, FCXCombatMessageData& OutNextPhaseRequest);

	// 바인드 등록 함수
	void BindTurnResponseDelegate(ACXCombatCharacterBase* Participant);
	// 모든 바인드 제거 함수
	void UnbindAllTurnResponseDelegates();

	// 한쪽 진영이 끝났는지 검사
	bool ShouldEndBattle();
	// 전투 종료
	void EndBattle();

	bool CheckAllActionEnded();
	bool CheckAllReturnEnded() const;
	void ResetDefenseTargets();

	// 페이즈 변경 함수
	void NextTurnPhase(ECXCombatPhase& PreviousPhase);

	// 매니저 -> 캐릭터
	void SendPhaseRequest(const FCXCombatMessageData& OutMessage);

	void HandleCombatSpeedChanged(const FOnAttributeChangeData& Data, TWeakObjectPtr<ACXCombatCharacterBase> Participant);

	void RefreshUpcomingTurns();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TArray<TObjectPtr<class ACXCombatCharacterBase>> PlayerCharacters;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TArray<TObjectPtr<class ACXCombatCharacterBase>> EnemyCharacters;

	// 대기중인 턴을 저장하는 최소 힙
	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TArray<FTurnOrder> TurnOrderQueue;

	// 큐에서 꺼내 현재 진행중인 턴
	UPROPERTY(VisibleAnywhere, Category = "Combat")
	FTurnOrder CurrentTurnOrder;

	// Track subscriptions separately from the living participant arrays.
	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TArray<TWeakObjectPtr<ACXCombatCharacterBase>> BoundParticipants;

	// 현재 턴 주체자
	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<class ACXCombatCharacterBase> CurrentTurnPlayer;

	// 현재 페이즈
	UPROPERTY(VisibleAnywhere, Category = "CombatData")
	ECXCombatPhase CurrentPhase = ECXCombatPhase::BeginPhase;

	UPROPERTY(VisibleAnywhere, Category = "CombatData")
	FCXCombatMessageData SavedMessage;

	TArray<TWeakObjectPtr<APawn>> TargetCandidates;
	int32 SelectedTargetIndex = INDEX_NONE;

	// 현재 턴부터 표시 순서대로 저장
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ACXCombatCharacterBase>> UpcomingTurns;



	// 순서 정리 기준 코스트
private:

	float BaseActionCost = 100.f;

	// 현재 패링동작중인 객체 수
	int32 PendingReactionCount = 0;

	// 현재 공격총 타격 수
	int32 CurrentAttackTotalHitCount = 0;

	// 이번 공격에 대한 캐릭터별 패링 성공 횟수
	TMap<TWeakObjectPtr<ACXCombatCharacterBase>, int32> ParrySuccessCounts;

	// 현재 턴이 시작된 전투상의 시간
	// 실제 플레이 시간이나 애니메이션 재생 시간과는 별개
	UPROPERTY(VisibleAnywhere, Category = "Combat")
	float CurrentCombatTime = 0.f;

	// -1이면 측정중인 전투가 없음
	double CombatStartTimeSeconds = -1.f;

	UPROPERTY(EditAnywhere, Category = "Combat|TurnOrder", meta = (ClampMin = "1", UIMin = "1"))
	int32 TurnPreviewCount = 8;
};

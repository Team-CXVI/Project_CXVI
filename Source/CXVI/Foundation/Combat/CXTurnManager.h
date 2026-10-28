// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "CombatMessage/CXCombatMessageData.h"

#include "CXTurnManager.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerInputEnabledChanged, ECXCombatInputMode);
//DECLARE_MULTICAST_DELEGATE(FCXOnBattleEnded);
DECLARE_MULTICAST_DELEGATE_OneParam(FCXOnBattleEnded, bool /*bVictory*/);

class ACXCombatCharacterBase;

USTRUCT()
struct FTurnOrder
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<ACXCombatCharacterBase> Character = nullptr;

	UPROPERTY()
	float NextActionTime = 0.f;
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

	void StopBattle() { ResetBattle(); }

private:

	// 초기화 함수.
	void ResetBattle();

	// 순서 정렬
	void SortTurnOrder();
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

	// 페이즈 변경 함수
	void NextTurnPhase(ECXCombatPhase& PreviousPhase);

	// 매니저 -> 캐릭터
	void SendPhaseRequest(const FCXCombatMessageData& OutMessage);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Battle")
	TArray<TObjectPtr<class ACXCombatCharacterBase>> PlayerCharacters;

	UPROPERTY(VisibleAnywhere, Category = "Battle")
	TArray<TObjectPtr<class ACXCombatCharacterBase>> EnemyCharacters;

	UPROPERTY(VisibleAnywhere, Category = "Battle")
	TArray<FTurnOrder> TurnOrderArray;

	// Track subscriptions separately from the living participant arrays.
	UPROPERTY(VisibleAnywhere, Category = "Battle")
	TArray<TWeakObjectPtr<ACXCombatCharacterBase>> BoundParticipants;

	// 현재 턴 주체자
	UPROPERTY(VisibleAnywhere, Category = "Battle")
	TObjectPtr<class ACXCombatCharacterBase> CurrentTurnPlayer;

	// 현재 페이즈
	UPROPERTY(VisibleAnywhere, Category = "CombatData")
	ECXCombatPhase CurrentPhase = ECXCombatPhase::BeginPhase;

	UPROPERTY(VisibleAnywhere, Category = "CombatData")
	FCXCombatMessageData SavedMessage;

	TArray<TWeakObjectPtr<APawn>> TargetCandidates;
	int32 SelectedTargetIndex = INDEX_NONE;

	// 순서 정리 기준 코스트
private:

	float BaseActionCost = 100.f;

	// 현재 패링동작중인 객체 수
	int32 PendingReactionCount = 0;
};

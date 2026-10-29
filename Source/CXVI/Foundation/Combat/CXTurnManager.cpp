// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Foundation/Combat/CXTurnManager.h"

#include "CXVI/Foundation/Combat/CXCombatMessageInterface.h"
#include "CXVI/Character/Combat/CXCombatCharacterBase.h"

#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXTurnManager)

UCXTurnManager::UCXTurnManager()
{
}

void UCXTurnManager::StartBattle(TArray<TObjectPtr<ACXCombatCharacterBase>> Players, TArray<TObjectPtr<ACXCombatCharacterBase>> Enemies)
{
	ResetBattle();

	// 플레이어와 몬스터를 순서배열에 다 합치기
	for (int32 Index = 0; Index < Players.Num(); ++Index)
	{
		if (!IsValid(Players[Index]))
		{
			continue;
		}

		FTurnOrder TurnOrder;
		TurnOrder.Character = Players[Index];
		TurnOrder.NextActionTime = BaseActionCost / Players[Index]->GetCombatSpeed();

		PlayerCharacters.Add(Players[Index]);
		BindTurnResponseDelegate(Players[Index]);
		TurnOrderArray.Add(TurnOrder);
	}

	for (int32 Index = 0; Index < Enemies.Num(); ++Index)
	{
		if (!IsValid(Enemies[Index]))
		{
			continue;
		}

		FTurnOrder TurnOrder;
		TurnOrder.Character = Enemies[Index];
		TurnOrder.NextActionTime = BaseActionCost / Enemies[Index]->GetCombatSpeed();

		EnemyCharacters.Add(Enemies[Index]);
		BindTurnResponseDelegate(Enemies[Index]);
		TurnOrderArray.Add(TurnOrder);
	}

	// 순서 정렬
	SortTurnOrder();

	FindNextPlayer();

	StartTurn();
}

void UCXTurnManager::ResetBattle()
{
	UnbindAllTurnResponseDelegates();
	PlayerCharacters.Reset();
	EnemyCharacters.Reset();
	TurnOrderArray.Reset();
	CurrentTurnPlayer = nullptr;
	CurrentAttackTotalHitCount = 0;
	ParrySuccessCounts.Reset();
	CurrentPhase = ECXCombatPhase::BeginPhase;
}

void UCXTurnManager::SortTurnOrder()
{
	if (TurnOrderArray.Num() <= 0)
	{
		return;
	}

	TurnOrderArray.StableSort([](const FTurnOrder& Left, const FTurnOrder& Right)
		{
			if (Left.NextActionTime != Right.NextActionTime)
			{
				return Left.NextActionTime < Right.NextActionTime;
			}

			return Left.Character->GetCombatSpeed() > Right.Character->GetCombatSpeed();
		}
	);
}

void UCXTurnManager::FindNextPlayer()
{
	CurrentTurnPlayer = nullptr;

	for (int32 Index = 0; Index < TurnOrderArray.Num(); ++Index)
	{
		if (IsValid(TurnOrderArray[Index].Character) && TurnOrderArray[Index].Character->GetIsCharacterAlive())
		{
			// 캐릭터가 살아있다면 턴 부여 죽어있다면 계속 검색
			CurrentTurnPlayer = TurnOrderArray[Index].Character;
			break;
		}
	}
}

void UCXTurnManager::StartTurn()
{
	if (!IsValid(CurrentTurnPlayer))
	{
		return;
	}

	CurrentAttackTotalHitCount = 0;
	ParrySuccessCounts.Reset();

	CurrentPhase = ECXCombatPhase::BeginPhase;

	SavedMessage.Phase = CurrentPhase;

	SendPhaseRequest(SavedMessage);
}

void UCXTurnManager::CompleteTurn()
{
	if (!IsValid(CurrentTurnPlayer))
	{
		return;
	}

	OnPlayerInputEnabledChanged.Broadcast(ECXCombatInputMode::Disabled);

	for (auto& TurnOrder : TurnOrderArray)
	{
		if (TurnOrder.Character == CurrentTurnPlayer)
		{
			TurnOrder.NextActionTime += BaseActionCost / TurnOrder.Character->GetCombatSpeed();
		}
	}

	CurrentTurnPlayer = nullptr;

	// 갱신된 정보로 다시 순서 정렬
	SortTurnOrder();

	FindNextPlayer();
	// 다음 턴 시작
	StartTurn();
}

void UCXTurnManager::HandleTurnResponse(const FCXCombatMessageData& Response)
{
	ACXCombatCharacterBase* Sender = Cast<ACXCombatCharacterBase>(Response.Character.Get());
	if (nullptr == Sender)
	{
		return;
	}

	if (Sender != CurrentTurnPlayer)
	{
		// 메시지가 왔는데 현재 턴플레이어가 아닌데 오는경우는 플레이어가 반격을 하는 상태

	}

	//SavedMessage.Character = Sender;
	SavedMessage.Targets.Reset();

	if (!ProcessTurnResponse(Response, SavedMessage))
	{
		return;
	}


	SendPhaseRequest(SavedMessage);
}

bool UCXTurnManager::ProcessTurnResponse(const FCXCombatMessageData& Response, FCXCombatMessageData& OutNextPhaseRequest)
{
	switch (Response.MessageType)
	{
	case ECXCombatMessageType::None:
	{
		// 요청하는게 없으므로 다음페이즈로 이동
		NextTurnPhase(CurrentPhase);
		OutNextPhaseRequest.Phase = CurrentPhase;
		break;
	}
	case ECXCombatMessageType::EnemyTargetsRequest:
	{
		// 적의 배열을 요청 플레이어가 보냈으면 적의 배열 / 적이 보냈으면 플레이어쪽의 배열
		ACXCombatCharacterBase* Sender = Cast<ACXCombatCharacterBase>(Response.Character.Get());
		if (nullptr == Sender)
		{
			// 요청한 자가 없음 -> 문제있음 
			// 일단 break 
			break;
		}

		// 플레이어쪽이면 true
		// 적쪽이면 false
		const bool bIsPlayerTeam = PlayerCharacters.Contains(Sender);

		// 배열에 추가전 초기화
		OutNextPhaseRequest.Targets.Reset();

		if (bIsPlayerTeam)
		{
			// 요청자가 플레이어쪽임
			for (int32 Index = 0; Index < EnemyCharacters.Num(); ++Index)
			{
				if (!IsValid(EnemyCharacters[Index]) && !EnemyCharacters[Index]->GetIsCharacterAlive())
				{
					// 죽은객체는 안보냄
					continue;
				}

				OutNextPhaseRequest.Targets.Add(EnemyCharacters[Index]);
			}
		}
		else
		{
			// 요청자가 적쪽
			for (int32 Index = 0; Index < PlayerCharacters.Num(); ++Index)
			{
				if (!PlayerCharacters[Index]->GetIsCharacterAlive())
				{
					// 죽은 객체
					continue;
				}

				OutNextPhaseRequest.Targets.Add(PlayerCharacters[Index]);
			}
		}

		// 배열과 함께 다음페이즈로 변경
		NextTurnPhase(CurrentPhase);
		OutNextPhaseRequest.Phase = CurrentPhase;


		break;
	}
	case ECXCombatMessageType::TeamTargetRequest:
	{
		// 적의 배열을 요청 플레이어가 보냈으면 적의 배열 / 적이 보냈으면 플레이어쪽의 배열
		ACXCombatCharacterBase* Sender = Cast<ACXCombatCharacterBase>(Response.Character.Get());
		if (nullptr == Sender)
		{
			// 요청한 자가 없음 -> 문제있음 
			// 일단 break 
			break;
		}

		// 플레이어쪽이면 true
		// 적쪽이면 false
		const bool bIsPlayerTeam = PlayerCharacters.Contains(Sender);

		// 배열에 추가전 초기화
		OutNextPhaseRequest.Targets.Reset();

		if (!bIsPlayerTeam)
		{
			// 요청자가 적쪽임
			for (int32 Index = 0; Index < EnemyCharacters.Num(); ++Index)
			{
				if (!EnemyCharacters[Index]->GetIsCharacterAlive())
				{
					// 죽은객체는 안보냄
					continue;
				}

				OutNextPhaseRequest.Targets.Add(EnemyCharacters[Index]);
			}
		}
		else
		{
			// 요청자가 플레이어쪽
			for (int32 Index = 0; Index < PlayerCharacters.Num(); ++Index)
			{
				if (!PlayerCharacters[Index]->GetIsCharacterAlive())
				{
					// 죽은 객체
					continue;
				}

				OutNextPhaseRequest.Targets.Add(PlayerCharacters[Index]);
			}
		}

		// 배열과 함께 다음페이즈로 변경
		NextTurnPhase(CurrentPhase);
		OutNextPhaseRequest.Phase = CurrentPhase;

		break;
	}

	case ECXCombatMessageType::SelectionCancelled:
	{
		// 행동직전 선택하는과정에서 돌아감
		CurrentPhase = ECXCombatPhase::SelectActionPhase;
		OutNextPhaseRequest.Phase = CurrentPhase;

		break;
	}

	case ECXCombatMessageType::SkippedTurn:
	{
		CurrentPhase = ECXCombatPhase::EndPhase;
		OutNextPhaseRequest.Phase = CurrentPhase;
		break;
	}

	case ECXCombatMessageType::Runaway:
	{
		EndBattle();
		return false;
	}

	case ECXCombatMessageType::Action:
	{
		// 행동 종료 통지는 액션 진행중에만 전환 판단에 사용
		if (CurrentPhase != ECXCombatPhase::ActionPhase)
		{
			return false;
		}

		if (ShouldEndBattle())
		{
			EndBattle();
			return false;
		}

		// 여기서 복귀페이즈로 가도되나 계속 검사
		if (!CheckAllActionEnded())
		{
			return false;
		}

		// 공격받는 상태 초기화
		ResetDefenseTargets();

		// 복귀 페이즈로 이동
		CurrentPhase = ECXCombatPhase::ReturnPhase;
		OutNextPhaseRequest.Phase = CurrentPhase;
		break;
	}

	case ECXCombatMessageType::Damaged:
	{
		if (!CurrentTurnPlayer->GetIsCharacterAlive())
		{
			// 죽었으면 전투가 끝났는지 검사
			if (ShouldEndBattle())
			{
				EndBattle();
				return false;
			}

			// 안끝낫으면 턴종료
			CurrentPhase = ECXCombatPhase::EndPhase;
			OutNextPhaseRequest.Phase = CurrentPhase;
		}

		NextTurnPhase(CurrentPhase);
		OutNextPhaseRequest.Phase = CurrentPhase;

		break;
	}

	case ECXCombatMessageType::TurnCompleted:
	{
		CompleteTurn();

		return false;
	}

	case ECXCombatMessageType::AttackStarted:
	{
		ACXCombatCharacterBase* Attacker = Cast<ACXCombatCharacterBase>(Response.Character.Get());
		if (!IsValid(Attacker) || Attacker != CurrentTurnPlayer || CurrentPhase != ECXCombatPhase::ActionPhase)
		{
			return false;
		}

		if (Response.TotalHitCount <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s : 공격의 타격 데이터가 비어있습니다"), *Attacker->GetName());
			return false;
		}

		CurrentAttackTotalHitCount = Response.TotalHitCount;

		// 새로운 공격이므로 이전 성공 기록 초기화.
		ParrySuccessCounts.Reset();

		UE_LOG(LogTemp, Display, TEXT("[공격등록] 공격자=%s, 총타격 수 =%d"), *Attacker->GetName(), CurrentAttackTotalHitCount);
		return false;
	}

	case ECXCombatMessageType::ParrySucceeded:
	{
		ACXCombatCharacterBase* Defender = Cast<ACXCombatCharacterBase>(Response.Character.Get());
		if (CurrentPhase != ECXCombatPhase::ActionPhase || CurrentAttackTotalHitCount <= 0 ||
			!IsValid(CurrentTurnPlayer) || Response.Attacker.Get() != CurrentTurnPlayer.Get() ||
			!IsValid(Defender) || !Defender->GetIsCharacterAlive() || !Defender->GetIsDefenseTarget())
		{
			return false;
		}

		const TWeakObjectPtr<ACXCombatCharacterBase> Key(Defender);

		int32& SuccessCount = ParrySuccessCounts.FindOrAdd(Key);
		++SuccessCount; 

		UE_LOG(LogTemp, Display, TEXT("[패링 성공] %s: %d/%d"), *Defender->GetName(), SuccessCount, CurrentAttackTotalHitCount);
		// 집계만 하고 페이즈는 진행X
		return false;
	}

	}

	
	return true;
}

void UCXTurnManager::BindTurnResponseDelegate(ACXCombatCharacterBase* Participant)
{
	ICXCombatMessageInterface* TurnParticipant = Cast<ICXCombatMessageInterface>(Participant);
	if (!TurnParticipant)
	{
		return;
	}

	FOnTurnPhaseFinished& Event = TurnParticipant->GetPhaseFinishedDelegate();
	Event.RemoveAll(this);
	Event.AddUObject(this, &UCXTurnManager::HandleTurnResponse);
	BoundParticipants.AddUnique(TWeakObjectPtr<ACXCombatCharacterBase>(Participant));
}

void UCXTurnManager::UnbindAllTurnResponseDelegates()
{
	for (const TWeakObjectPtr<ACXCombatCharacterBase>& Participant : BoundParticipants)
	{
		if (ICXCombatMessageInterface* TurnParticipant = Cast<ICXCombatMessageInterface>(Participant.Get()))
		{
			TurnParticipant->GetPhaseFinishedDelegate().RemoveAll(this);
		}
	}

	BoundParticipants.Reset();
}

bool UCXTurnManager::ShouldEndBattle()
{
	bool bIsAlivePlayer = false;
	bool bIsAliveEnemy = false;

	for (int32 Index = 0; Index < PlayerCharacters.Num(); ++Index)
	{
		if (PlayerCharacters[Index]->GetIsCharacterAlive())
		{
			// 살아있는 플레이어가 하나라도 있으면 true
			bIsAlivePlayer = true;
		}
	}

	for (int32 Index = 0; Index < EnemyCharacters.Num(); ++Index)
	{
		if (EnemyCharacters[Index] && EnemyCharacters[Index]->GetIsCharacterAlive())
		{
			bIsAliveEnemy = true;
		}
	}

	return !bIsAlivePlayer || !bIsAliveEnemy;
}                                                                                        

void UCXTurnManager::EndBattle()
{
	// TODO : 전투 결과 조건 정립
	// ResetBattle이 전투원 배열을 비우므로 그 전에 결과 판정
	// 승리 = 살아 있는 플레이어가 있고 살아 있는 적이 없음 (도망은 적이 살아 있으므로 패배 처리)
	bool bIsAlivePlayer = false;
	for (const TObjectPtr<ACXCombatCharacterBase>& Player : PlayerCharacters)
	{
		if (IsValid(Player) && Player->GetIsCharacterAlive())
		{
			bIsAlivePlayer = true;
			break;
		}
	}

	bool bIsAliveEnemy = false;
	for (const TObjectPtr<ACXCombatCharacterBase>& Enemy : EnemyCharacters)
	{
		if (IsValid(Enemy) && Enemy->GetIsCharacterAlive())
		{
			bIsAliveEnemy = true;
			break;
		}
	}

	const bool bVictory = bIsAlivePlayer && !bIsAliveEnemy;

	ResetBattle();

	OnBattleEnded.Broadcast(bVictory);
}

bool UCXTurnManager::CheckAllActionEnded()
{
	if (!IsValid(CurrentTurnPlayer))
	{
		// 현재 턴 주체자가 없다? 그럴수가 있나 일단 모든 턴이 끝났다고 가정
		return true;
	}

	if (CurrentTurnPlayer->GetDoingAction())
	{
		// 현재 턴 주체자가 행동중 
		return false;
	}

	for (int32 Index = 0; Index < PlayerCharacters.Num(); ++Index)
	{
		if (IsValid(PlayerCharacters[Index]) && PlayerCharacters[Index]->GetIsCharacterAlive() && PlayerCharacters[Index]->GetDoingAction())
		{
			return false;
		}
	}

	for (int32 Index = 0; Index < EnemyCharacters.Num(); ++Index)
	{
		if (IsValid(EnemyCharacters[Index]) && EnemyCharacters[Index]->GetIsCharacterAlive() && EnemyCharacters[Index]->GetDoingAction())
		{
			return false;
		}
	}

	// 여기 까지왔으면 모든 행동하는 사람이 없다
	return true;
}

void UCXTurnManager::ResetDefenseTargets()
{
	for (int32 Index = 0; Index < PlayerCharacters.Num(); ++Index)
	{
		if (IsValid(PlayerCharacters[Index]) && PlayerCharacters[Index]->GetIsDefenseTarget())
		{
			PlayerCharacters[Index]->SetIsDefenseTarget(false);
		}
	}

	for (int32 Index = 0; Index < EnemyCharacters.Num(); ++Index)
	{
		if (IsValid(EnemyCharacters[Index]) && EnemyCharacters[Index]->GetIsDefenseTarget())
		{
			EnemyCharacters[Index]->SetIsDefenseTarget(false);
		}
	}
}

void UCXTurnManager::NextTurnPhase(ECXCombatPhase& PreviousPhase)
{
	uint8 NextPhase = (static_cast<uint8>(PreviousPhase) + 1) % static_cast<uint8>(ECXCombatPhase::PhaseCount);

	PreviousPhase = static_cast<ECXCombatPhase>(NextPhase);
}

void UCXTurnManager::SendPhaseRequest(const FCXCombatMessageData& OutMessage)
{
	ECXCombatInputMode InputMode = ECXCombatInputMode::Disabled;

	if (IsValid(CurrentTurnPlayer))
	{
		const bool bSelecting =
			OutMessage.Phase == ECXCombatPhase::SelectActionPhase ||
			OutMessage.Phase == ECXCombatPhase::SelectTargetPhase;

		if (PlayerCharacters.Contains(CurrentTurnPlayer) && bSelecting)
		{
			InputMode = ECXCombatInputMode::Command;
		}
		else if (EnemyCharacters.Contains(CurrentTurnPlayer) && OutMessage.Phase == ECXCombatPhase::ActionPhase)
		{
			ResetDefenseTargets();

			ACXCombatCharacterBase* Target = Cast<ACXCombatCharacterBase>(CurrentTurnPlayer->GetSelectedCombatTarget());

			if (IsValid(Target) && Target->GetIsCharacterAlive() && PlayerCharacters.Contains(Target))
			{
				Target->SetIsDefenseTarget(true);

				UE_LOG(LogTemp, Display, TEXT("방어 대상 지정 : %s"), *Target->GetName());
			}

			InputMode = ECXCombatInputMode::Reaction;
		}
	}

	OnPlayerInputEnabledChanged.Broadcast(InputMode);

	if (IsValid(CurrentTurnPlayer))
	{
		CurrentTurnPlayer->CombatMessageRequest(OutMessage);
	}
}

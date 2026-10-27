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

	SavedMessage.Character = Sender;
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
		if (ShouldEndBattle())
		{
			EndBattle();
			return false;
		}

		// 리액션 페이즈로 이동
		NextTurnPhase(CurrentPhase);
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
	ResetBattle();

	OnBattleEnded.Broadcast();
	// 다시 필드로 돌아가야함
	// 배틀 매니저에게 요청
	/*if (AGASGameMode* GameMode = GetWorld()->GetAuthGameMode<AGASGameMode>())
	{
		GameMode->RequestEndBattle();
	}*/
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
			InputMode = ECXCombatInputMode::Reaction;
		}
	}

	OnPlayerInputEnabledChanged.Broadcast(InputMode);

	if (IsValid(CurrentTurnPlayer))
	{
		CurrentTurnPlayer->CombatMessageRequest(OutMessage);
	}
}

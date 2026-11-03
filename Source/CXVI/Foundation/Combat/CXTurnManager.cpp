// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Foundation/Combat/CXTurnManager.h"

#include "CXVI/Foundation/Combat/CXCombatMessageInterface.h"
#include "CXVI/Character/Combat/CXCombatCharacterBase.h"
#include "AbilitySystem/Attributes/Combat/CXCombatAttributeSetBase.h"

#include "GameplayEffectTypes.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXTurnManager)

namespace
{
	struct FTurnOrderPriority
	{
		bool operator()(const FTurnOrder& Left, const FTurnOrder& Right) const
		{
			// 행동 시간이 빠른 캐릭터 우선
			if (Left.NextActionTime != Right.NextActionTime)
			{
				return Left.NextActionTime < Right.NextActionTime;
			}

			// 시간이 같으면 속도가 높은 캐릭터 우선
			if (Left.CombatSpeed != Right.CombatSpeed)
			{
				return Left.CombatSpeed > Right.CombatSpeed;
			}

			// 시간과 속도도 같으면 최초 등록 순서
			return Left.RegistrationOrder < Right.RegistrationOrder;
		}
	};
}

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
		TurnOrder.CombatSpeed = FMath::Max(Players[Index]->GetCombatSpeed(), 1.f);
		TurnOrder.NextActionTime = BaseActionCost / TurnOrder.CombatSpeed;
		TurnOrder.RegistrationOrder = TurnOrderQueue.Num();

		PlayerCharacters.Add(Players[Index]);
		BindTurnResponseDelegate(Players[Index]);
		TurnOrderQueue.HeapPush(TurnOrder, FTurnOrderPriority{});
	}

	for (int32 Index = 0; Index < Enemies.Num(); ++Index)
	{
		if (!IsValid(Enemies[Index]))
		{
			continue;
		}

		FTurnOrder TurnOrder;
		TurnOrder.Character = Enemies[Index];
		TurnOrder.CombatSpeed = FMath::Max(Enemies[Index]->GetCombatSpeed(), 1.f);
		TurnOrder.NextActionTime = BaseActionCost / TurnOrder.CombatSpeed;
		TurnOrder.RegistrationOrder = TurnOrderQueue.Num();

		EnemyCharacters.Add(Enemies[Index]);
		BindTurnResponseDelegate(Enemies[Index]);
		TurnOrderQueue.HeapPush(TurnOrder, FTurnOrderPriority{});
	}

	// 순서 정렬
	//SortTurnOrder();

	FindNextPlayer();

	// 첫턴을 시작하기 직전에 측정 시작
	if (IsValid(CurrentTurnPlayer))
	{
		if (const UWorld* World = GetWorld())
		{
			CombatStartTimeSeconds = World->GetTimeSeconds();
		}
	}

	StartTurn();
}

double UCXTurnManager::GetCombatElapsedSeconds() const
{
	const UWorld* World = GetWorld();

	if (!World || CombatStartTimeSeconds < 0.f)
	{
		return 0.f;
	}

	return FMath::Max(0.f, World->GetTimeSeconds() - CombatStartTimeSeconds);
}

void UCXTurnManager::SetTurnPreviewCount(int32 NewCount)
{
	TurnPreviewCount = FMath::Max(NewCount, 1);
	RefreshUpcomingTurns();
}

void UCXTurnManager::ResetBattle()
{
	UnbindAllTurnResponseDelegates();
	PlayerCharacters.Reset();
	EnemyCharacters.Reset();
	TurnOrderQueue.Reset();
	CurrentTurnOrder = FTurnOrder{};
	CurrentTurnPlayer = nullptr;
	CurrentAttackTotalHitCount = 0;
	ParrySuccessCounts.Reset();
	CurrentCombatTime = 0.f;
	UpcomingTurns.Reset();
	CombatStartTimeSeconds = -1.f;
	CurrentPhase = ECXCombatPhase::BeginPhase;
}

//void UCXTurnManager::SortTurnOrder()
//{
//	if (TurnOrderArray.Num() <= 0)
//	{
//		return;
//	}
//
//	TurnOrderArray.StableSort([](const FTurnOrder& Left, const FTurnOrder& Right)
//		{
//			if (Left.NextActionTime != Right.NextActionTime)
//			{
//				return Left.NextActionTime < Right.NextActionTime;
//			}
//
//			return Left.Character->GetCombatSpeed() > Right.Character->GetCombatSpeed();
//		}
//	);
//}
//
void UCXTurnManager::FindNextPlayer()
{
	CurrentTurnPlayer = nullptr;
	CurrentTurnOrder = FTurnOrder();

	// 사망한 캐릭터의 항목은 잠시 꺼냈다가 다시 보관
	TArray<FTurnOrder> SkippedTurns;
	
	while (!TurnOrderQueue.IsEmpty())
	{
		FTurnOrder NextTurn;

		TurnOrderQueue.HeapPop(NextTurn, FTurnOrderPriority{}, EAllowShrinking::No);

		ACXCombatCharacterBase* Character = NextTurn.Character.Get();

		// 파괴된 객체의 항목은 제거
		if (!IsValid(Character))
		{
			continue;
		}

		if (!Character->GetIsCharacterAlive())
		{
			SkippedTurns.Add(NextTurn);
			continue;
		}

		CurrentTurnOrder = NextTurn;
		CurrentTurnPlayer = Character;

		// 선택된 턴의 예정시간까지 전투시간 진행
		// 과거 시간이 남아 있어도 전투 시간이 뒤로 가지 않도록 제한
		CurrentCombatTime = FMath::Max(CurrentCombatTime, NextTurn.NextActionTime);

		CurrentTurnOrder.NextActionTime = CurrentCombatTime;

		break;

	}

	for (const FTurnOrder& SkippedTurn : SkippedTurns)
	{
		TurnOrderQueue.HeapPush(SkippedTurn, FTurnOrderPriority{});
	}
}

void UCXTurnManager::StartTurn()
{
	if (!IsValid(CurrentTurnPlayer))
	{
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("[턴 시작] %s / 행동 시간=%.3f / 속도=%.1f"),
		*GetNameSafe(CurrentTurnPlayer.Get()),
		CurrentTurnOrder.NextActionTime,
		CurrentTurnOrder.CombatSpeed
	);

	RefreshUpcomingTurns();

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

	// 이번턴 종료 시점의 속도로 다음 행동 간격 계산.
	CurrentTurnOrder.CombatSpeed = FMath::Max(CurrentTurnPlayer->GetCombatSpeed(), 1.f);

	// 현재 전투 시간을 기준으로 다음 행동 예약
	CurrentTurnOrder.NextActionTime = CurrentCombatTime + BaseActionCost / CurrentTurnOrder.CombatSpeed;

	// 현재 턴을 다시 대기 큐에 삽입
	TurnOrderQueue.HeapPush(CurrentTurnOrder, FTurnOrderPriority{});

	CurrentTurnPlayer = nullptr;
	CurrentTurnOrder = FTurnOrder{};

	FindNextPlayer();
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
		CurrentPhase = CheckAllReturnEnded() ? ECXCombatPhase::EndPhase : ECXCombatPhase::ReturnPhase;
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

		if (SuccessCount == CurrentAttackTotalHitCount)
		{
			// 모든 공격을 패리했으므로 보상.
			Defender->SetCounterAttackPending(true);
		}

		return false;
	}
	case ECXCombatMessageType::ReturnCompleted:
	{
		if (CurrentPhase != ECXCombatPhase::ReturnPhase || Response.Phase != ECXCombatPhase::ReturnPhase)
		{
			return false;
		}

		if (!CheckAllReturnEnded())
		{
			// 다른 캐릭터 복귀중
			return false;
		}

		CurrentPhase = ECXCombatPhase::EndPhase;
		OutNextPhaseRequest.Phase = CurrentPhase;

		break;
	}

	}

	
	return true;
}

void UCXTurnManager::BindTurnResponseDelegate(ACXCombatCharacterBase* Participant)
{
	if (!IsValid(Participant))
	{
		return;
	}

	ICXCombatMessageInterface* TurnParticipant = Cast<ICXCombatMessageInterface>(Participant);
	if (!TurnParticipant)
	{
		return;
	}

	FOnTurnPhaseFinished& Event = TurnParticipant->GetPhaseFinishedDelegate();
	Event.RemoveAll(this);
	Event.AddUObject(this, &UCXTurnManager::HandleTurnResponse);
	BoundParticipants.AddUnique(TWeakObjectPtr<ACXCombatCharacterBase>(Participant));

	UAbilitySystemComponent* ASC = Participant->GetAbilitySystemComponent();

	if (IsValid(ASC))
	{
		auto& SpeedChanged = ASC->GetGameplayAttributeValueChangeDelegate(UCXCombatAttributeSetBase::GetCombatSpeedAttribute());

		// 중복 등록 방지.
		SpeedChanged.RemoveAll(this);

		SpeedChanged.AddUObject(this, &UCXTurnManager::HandleCombatSpeedChanged, TWeakObjectPtr<ACXCombatCharacterBase>(Participant));
	}
}

void UCXTurnManager::UnbindAllTurnResponseDelegates()
{
	for (const TWeakObjectPtr<ACXCombatCharacterBase>& WeakParticipant : BoundParticipants)
	{
		ACXCombatCharacterBase* Participant = WeakParticipant.Get();

		if (!IsValid(Participant))
		{
			continue;
		}

		if (ICXCombatMessageInterface* TurnParticipant = Cast<ICXCombatMessageInterface>(Participant))
		{
			TurnParticipant->GetPhaseFinishedDelegate().RemoveAll(this);
		}

		if (UAbilitySystemComponent* ASC = Participant->GetAbilitySystemComponent(); IsValid(ASC))
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UCXCombatAttributeSetBase::GetCombatSpeedAttribute()).RemoveAll(this);
			
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

	// 초기화 전에 결과에 사용할 시간을 확보
	const double ElapsedSeconds = GetCombatElapsedSeconds();

	UE_LOG(LogTemp, Display, TEXT("[전투 종료] 승리=%d, 경과 시간=%.2f"),
		bVictory, ElapsedSeconds);

	ResetBattle();

	OnBattleEnded.Broadcast(bVictory, ElapsedSeconds);
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

bool UCXTurnManager::CheckAllReturnEnded() const
{
	for (const auto& Character : PlayerCharacters)
	{
		if (IsValid(Character) && Character->GetWaitingForReturn())
		{
			return false;
		}
	}

	for (const auto& Character : EnemyCharacters)
	{
		if (IsValid(Character) && Character->GetWaitingForReturn())
		{
			return false;
		}
	}

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
		else if (PlayerCharacters.Contains(CurrentTurnPlayer) && OutMessage.Phase == ECXCombatPhase::ActionPhase)
		{
			InputMode = ECXCombatInputMode::JustTiming;
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

	if (OutMessage.Phase == ECXCombatPhase::ReturnPhase)
	{
		// 콜백중 SavedMessage가 바뀔수 있으므로 복사해서 사용.
		const FCXCombatMessageData ReturnMessage = OutMessage;

		for (const auto& Character : PlayerCharacters)
		{
			if (IsValid(Character) && Character->GetWaitingForReturn())
			{
				Character->CombatMessageRequest(ReturnMessage);
			}
		}

		for (const auto& Character : EnemyCharacters)
		{
			if (IsValid(Character) && Character->GetWaitingForReturn())
			{
				Character->CombatMessageRequest(ReturnMessage);
			}
		}

		return;
	}

	if (IsValid(CurrentTurnPlayer))
	{
		CurrentTurnPlayer->CombatMessageRequest(OutMessage);
	}
}

void UCXTurnManager::HandleCombatSpeedChanged(const FOnAttributeChangeData& Data, TWeakObjectPtr<ACXCombatCharacterBase> Participant)
{
	ACXCombatCharacterBase* Character = Participant.Get();

	if (!IsValid(Character))
	{
		return;
	}

	const float NewSpeed = FMath::Max(Data.NewValue, 1.f);

	// 현재 행동자는 대기 중이 아니므로 예정 시간을 당기지않음.
	// 자신의 턴 종료시 최신 속도로 다음 행동을 예약
	if (Character == CurrentTurnPlayer.Get())
	{
		CurrentTurnOrder.CombatSpeed = NewSpeed;

		RefreshUpcomingTurns();

		return;
	}

	FTurnOrder* QueuedTurn = TurnOrderQueue.FindByPredicate(
		[Character](const FTurnOrder& Turn)
		{
			return Turn.Character.Get() == Character;
		}
	);

	if (!QueuedTurn)
	{
		return;
	}

	// 이예정시간을 계산할때 사용했던 속도.
	const float OldSpeed = FMath::Max(QueuedTurn->CombatSpeed, 1.f);

	if (OldSpeed == NewSpeed)
	{
		return;
	}

	const float OldActionTime = QueuedTurn->NextActionTime;

	const float RemainingTime = FMath::Max(OldActionTime - CurrentCombatTime, 0.f);

	// 이미 기다린 진행도는 유지하고 남은 시간만 변경.
	QueuedTurn->NextActionTime = CurrentCombatTime + RemainingTime * (OldSpeed / NewSpeed);

	QueuedTurn->CombatSpeed = NewSpeed;

	UE_LOG(LogTemp, Display, TEXT("[턴 속도 변경] %s / 속도 %.1f -> %.1f / 예정 시간 %.3f -> %.3f / 현재 시간 %.3f"),
		*GetNameSafe(Character),
		OldSpeed,
		NewSpeed,
		OldActionTime,
		QueuedTurn->NextActionTime,
		CurrentCombatTime
	);

	// 내부 항목의 우선순위가 변경됐으므로 힙 재구성
	TurnOrderQueue.Heapify(FTurnOrderPriority{});

	RefreshUpcomingTurns();
}

void UCXTurnManager::RefreshUpcomingTurns()
{
	UpcomingTurns.Reset();

	ACXCombatCharacterBase* CurrentPlayer = CurrentTurnPlayer.Get();
	if (!IsValid(CurrentPlayer))
	{
		OnUpcomingTurnsChanged.Broadcast();
		return;
	}

	const int32 PreviewCount = FMath::Max(TurnPreviewCount, 1);
	TArray<FTurnOrder> PreviewQueue = TurnOrderQueue;
	float PreviewTime = CurrentCombatTime;

	if (CurrentPlayer->GetIsCharacterAlive())
	{
		// 이미 시작된 턴은 첫 번째 칸에 표시
		UpcomingTurns.Emplace(CurrentPlayer);

		// 현재 행동자는 대기 큐에 없으므로 다음 행동을 임시 큐에 예약
		FTurnOrder NextCurrentTurn = CurrentTurnOrder;
		NextCurrentTurn.NextActionTime = PreviewTime + BaseActionCost / NextCurrentTurn.CombatSpeed;

		PreviewQueue.HeapPush(NextCurrentTurn, FTurnOrderPriority{});
	}

	while (UpcomingTurns.Num() < PreviewCount && !PreviewQueue.IsEmpty())
	{
		FTurnOrder NextTurn;
		PreviewQueue.HeapPop(
			NextTurn,
			FTurnOrderPriority{},
			EAllowShrinking::No
		);

		ACXCombatCharacterBase* Character = NextTurn.Character.Get();
		if (!IsValid(Character) || !Character->GetIsCharacterAlive())
		{
			continue;
		}

		UpcomingTurns.Emplace(Character);

		// 방금 예측한 턴까지 임시 시간을 진행하고 이후 행동을 예약
		PreviewTime = FMath::Max(PreviewTime, NextTurn.NextActionTime);
		NextTurn.NextActionTime =
			PreviewTime + BaseActionCost / NextTurn.CombatSpeed;

		PreviewQueue.HeapPush(NextTurn, FTurnOrderPriority{});
	}

	TArray<FString> TurnNames;
	for (const auto& Turn : UpcomingTurns)
	{
		TurnNames.Add(GetNameSafe(Turn.Get()));
	}

	UE_LOG(LogTemp, Display, TEXT("[턴 예측] %d개 | %s"),
		UpcomingTurns.Num(),
		*FString::Join(TurnNames, TEXT(" -> ")));

	OnUpcomingTurnsChanged.Broadcast();
}

// Fill out your copyright notice in the Description page of Project Settings.

#include "CXVI/Foundation/Combat/CXCombatManagerSubsystem_V2.h"

#include "CXVI/Actor/Combat/CXCombatArena.h"
#include "CXVI/Character/Combat/CXCombatCharacterBase.h"
#include "CXVI/Character/Combat/CXCombatCharacterPlayer.h"
#include "CXVI/Character/Combat/CXCommandCharacter.h"
#include "CXVI/Foundation/Combat/CXTurnManager.h"
#include "CXVI/Foundation/Combat/CombatMessage/CXCombatMessageData.h"

#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"
#include "Foundation/Scene/CXSceneSubsystem.h"
#include "Level/CXLevelTransitionSubsystem.h"
#include "Level/CXLevelDefinition.h"
#include "Tags/CXGameplayTags.h"

#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatManagerSubsystem_V2)

DEFINE_LOG_CATEGORY(LogCXCombat);

UCXCombatManagerSubsystem_V2& UCXCombatManagerSubsystem_V2::Get(const UObject* WorldContextObject)
{
	UCXCombatManagerSubsystem_V2* Subsystem = TryGet(WorldContextObject);
	check(Subsystem);
	return *Subsystem;
}

UCXCombatManagerSubsystem_V2* UCXCombatManagerSubsystem_V2::TryGet(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	return World ? World->GetSubsystem<UCXCombatManagerSubsystem_V2>() : nullptr;
}

void UCXCombatManagerSubsystem_V2::PostInitialize()
{
	Super::PostInitialize();

	TurnManager = NewObject<UCXTurnManager>(this);

	TurnManager->OnBattleEnded_V2.AddUObject(this, &ThisClass::HandleBattleEnded);
	TurnManager->OnPlayerInputEnabledChanged.AddUObject(this, &ThisClass::HandlePlayerInputModeChanged);
}

bool UCXCombatManagerSubsystem_V2::RequestStartBattle(FName InEncounterId, const TArray<TSubclassOf<ACXCombatCharacterBase>>& InPlayerClasses, const TArray<TSubclassOf<ACXCombatCharacterBase>>& InEnemyClasses)
{
	if (!CanStartBattle(InEncounterId, InPlayerClasses, InEnemyClasses))
	{
		return false;
	}

	UCXSceneDefinition* CombatScene = GetCombatSceneDefinition();
	if (nullptr == CombatScene)
	{
		UE_LOG(LogCXCombat, Warning, TEXT("전투 요청 거절 : 현재 레벨 정의에 CombatScene이 없습니다."));
		return false;
	}

	FCXCombatEnterParams Params;
	Params.EncounterId = InEncounterId;
	Params.PlayerClasses = InPlayerClasses;
	Params.EnemyClasses = InEnemyClasses;

	// 전환 설정에 따라 Push 안에서 OnEnter -> BeginBattle이 바로 호출될 수 있으므로 Push 전에 상태 변경
	// Push 이후 OnEnter 전까지 들어오는 재요청도 이 상태로 거절
	CombatState = ECXCombatState::Entering;

	// 전환 중이면 PushScene이 거부함
	if (!UCXSceneSubsystem::Get(this).PushScene(CombatScene, FCXSceneContext::Make(Params)))
	{
		CombatState = ECXCombatState::Idle;
		return false;
	}

	UE_LOG(LogCXCombat, Log, TEXT("전투 진입 요청 : %s"), *InEncounterId.ToString());
	return true;
}

bool UCXCombatManagerSubsystem_V2::BeginBattle(const FCXBattleSetup& InSetup)
{
	// RequestStartBattle을 거치지 않은 진입(디버그 Scene Push 등)은 시작하지 않음
	if (ECXCombatState::Entering != CombatState)
	{
		UE_LOG(LogCXCombat, Warning, TEXT("BeginBattle 무시 : 진입 요청 상태가 아닙니다."));
		return false;
	}

	ACXCommandCharacter* CommandPawn = InSetup.CommandPawn;
	APlayerController* PlayerController = IsValid(CommandPawn) ? Cast<APlayerController>(CommandPawn->GetController()) : nullptr;
	if (!IsValid(TurnManager) || !IsValid(InSetup.Arena) || nullptr == PlayerController)
	{
		UE_LOG(LogCXCombat, Warning, TEXT("BeginBattle 실패 : 아레나 또는 빙의된 커맨드 Pawn이 없습니다."));
		return false;
	}

	if (InSetup.Players.IsEmpty() || InSetup.Enemies.IsEmpty())
	{
		UE_LOG(LogCXCombat, Warning, TEXT("BeginBattle 실패 : 전투원이 없습니다."));
		return false;
	}

	CombatArena = InSetup.Arena;
	WeakCommandCharacter = CommandPawn;

	// 빙의는 전투 Scene(UCXGameScene::OnEnter)이 먼저 처리함 -> 파티 등록만
	for (ACXCombatCharacterBase* Character : InSetup.Players)
	{
		if (ACXCombatCharacterPlayer* Player = Cast<ACXCombatCharacterPlayer>(Character))
		{
			CommandPawn->AddCommandCharacter(Player);
		}
	}

	PlayerController->SetViewTargetWithBlend(InSetup.Arena.Get(), 0.f);

	// StartBattle 안에서 입력 모드 변경 / 종료가 바로 일어날 수 있으므로 먼저 상태 변경
	CombatState = ECXCombatState::InBattle;
	TurnManager->StartBattle(InSetup.Players, InSetup.Enemies);

	return true;
}

void UCXCombatManagerSubsystem_V2::EndBattle()
{
	if (ECXCombatState::Idle == CombatState)
	{
		return;
	}

	// 전투원을 파괴하기 전에 턴 진행과 바인딩 정리 (전투원 정리는 전투 Scene이 아레나에 요청)
	if (IsValid(TurnManager))
	{
		TurnManager->StopBattle();
	}

	CombatArena.Reset();
	WeakCommandCharacter.Reset();

	CombatState = ECXCombatState::Idle;
}

bool UCXCombatManagerSubsystem_V2::CanStartBattle(FName InEncounterId, const TArray<TSubclassOf<ACXCombatCharacterBase>>& InPlayerClasses, const TArray<TSubclassOf<ACXCombatCharacterBase>>& InEnemyClasses) const
{
	if (ECXCombatState::Idle != CombatState)
	{
		UE_LOG(LogCXCombat, Warning, TEXT("전투 요청 거절 : 이미 전투 중입니다."));
		return false;
	}

	if (!IsValid(TurnManager))
	{
		return false;
	}

	// TODO : 인카운터 판단 기준이 정해지면 추가 (현재는 None 여부만 확인)
	if (InEncounterId.IsNone())
	{
		UE_LOG(LogCXCombat, Warning, TEXT("전투 요청 거절 : EncounterId가 없습니다."));
		return false;
	}

	if (InPlayerClasses.IsEmpty() || InEnemyClasses.IsEmpty())
	{
		UE_LOG(LogCXCombat, Warning, TEXT("전투 요청 거절 : 파티 정보가 비어 있습니다."));
		return false;
	}

	const UCXSceneSubsystem* SceneSubsystem = UCXSceneSubsystem::TryGet(this);
	if (nullptr == SceneSubsystem || SceneSubsystem->IsTransitioning())
	{
		UE_LOG(LogCXCombat, Warning, TEXT("전투 요청 거절 : Scene 전환 중이거나 Scene Subsystem이 없습니다."));
		return false;
	}

	return true;
}

UCXSceneDefinition* UCXCombatManagerSubsystem_V2::GetCombatSceneDefinition() const
{
	const UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this);
	const UCXLevelDefinition* LevelDefinition = LevelTransition ? LevelTransition->GetCurrentLevelDefinition() : nullptr;
	return LevelDefinition ? LevelDefinition->CombatScene.Get() : nullptr;
}

void UCXCombatManagerSubsystem_V2::HandleBattleEnded(bool bInVictory)
{
	if (ECXCombatState::InBattle != CombatState)
	{
		return;
	}

	// 종료 알림만 방송. 필드 복귀와 정리 시점은 받는 쪽(전투 Scene)이 결정
	FCXCombatEndMessage Message;
	Message.bVictory = bInVictory;

	UCXGameplayMessageSubsystem::Get(this).BroadcastMessage(CXVIGameplayTags::Message_Combat_End, Message);
}

void UCXCombatManagerSubsystem_V2::HandlePlayerInputModeChanged(bool bAllowed)
{
	ACXCommandCharacter* CommandCharacter = WeakCommandCharacter.Get();
	if (!IsValid(CommandCharacter))
	{
		return;
	}

	CommandCharacter->SetCombatInputMode(bAllowed ? ECombatInputMode::Command : ECombatInputMode::Disabled);
}

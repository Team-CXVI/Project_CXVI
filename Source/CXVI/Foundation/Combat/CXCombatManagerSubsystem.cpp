// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Foundation/Combat/CXCombatManagerSubsystem.h"

#include "CXVI/Character/Combat/CXCombatCharacterPlayer.h"
#include "CXVI/Character/Combat/CXCombatCharacterEnemy.h"
#include "CXVI/Character/Combat/CXCombatCharacterBase.h"
#include "CXVI/Character/Combat/CXCommandCharacter.h"
#include "CXVI/Actor/Combat/CXTestCombatArena.h"

#include "CXVI/Foundation/Combat/CXTurnManager.h"
#include "CXVI/Foundation/Combat/CXArenaManager.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/Level.h"
#include "Engine/LatentActionManager.h"
#include "Engine/LevelStreaming.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatManagerSubsystem)

UCXCombatManagerSubsystem::UCXCombatManagerSubsystem()
{
	
}

void UCXCombatManagerSubsystem::RequestStartBattle(TArray<TSubclassOf<class ACXCombatCharacterBase>> Players, TArray<TSubclassOf<class ACXCombatCharacterBase>> Enemies, APlayerController* PlayerController)
{
	// 로드 중 또는 전투 중 재진입 방지.
	if (bBattleRequested ||
		!IsValid(TurnManager) ||
		!IsValid(ArenaManager) ||
		!IsValid(PlayerController))
	{
		return;
	}

	if (Players.IsEmpty() || Enemies.IsEmpty())
	{
		return;
	}

	ULevelStreaming* Streaming =
		UGameplayStatics::GetStreamingLevel(this, CombatLevelName);

	if (!Streaming)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("전투 서브레벨이 등록되어 있지 않습니다."));
		return;
	}

	StoredFieldPawn = PlayerController->GetPawn();
	StoredPlayerController = PlayerController;

	PendingSetupData.PlayerClasses = MoveTemp(Players);
	PendingSetupData.EnemyClasses = MoveTemp(Enemies);

	TestCombatArena.Reset();

	bBattleRequested = true;
	bLoadingCombat = true;

	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;
	LatentInfo.ExecutionFunction =
		GET_FUNCTION_NAME_CHECKED(
			UCXCombatManagerSubsystem,
			HandleArenaLevelReady
		);
	LatentInfo.Linkage = 0;
	LatentInfo.UUID = 1;

	UGameplayStatics::LoadStreamLevel(
		this,
		CombatLevelName,
		true,
		false,
		LatentInfo
	);



}

void UCXCombatManagerSubsystem::RequestEndBattle()
{
	// 이 예시는 로드 중 취소는 처리하지 않음.
	if (!bBattleRequested || bLoadingCombat)
	{
		return;
	}

	// 캐릭터를 파괴하기 전에 턴 진행과 바인딩 정리.
	if (IsValid(TurnManager))
	{
		TurnManager->StopBattle();
	}

	if (IsValid(StoredPlayerController) &&
		IsValid(StoredFieldPawn))
	{
		StoredPlayerController->Possess(StoredFieldPawn);
		StoredPlayerController->SetViewTargetWithBlend(
			StoredFieldPawn.Get(), 0.3f);
	}

	if (IsValid(ArenaManager))
	{
		ArenaManager->CleanupArena();
	}

	if (ULevelStreaming* Streaming =
		UGameplayStatics::GetStreamingLevel(this, CombatLevelName))
	{
		Streaming->SetShouldBeVisible(false);
	}

	TestCombatArena.Reset();
	PendingSetupData = FCXArenaSetupData();

	StoredFieldPawn = nullptr;
	StoredPlayerController = nullptr;

	bBattleRequested = false;
}

void UCXCombatManagerSubsystem::PostInitialize()
{
	Super::PostInitialize();

	TurnManager = NewObject<UCXTurnManager>(this);
	ArenaManager = NewObject<UCXArenaManager>(this);

	ArenaManager->OnArenaReady.AddUObject(this, &UCXCombatManagerSubsystem::HandleArenaReady);
	TurnManager->OnBattleEnded.AddUObject(this, &UCXCombatManagerSubsystem::RequestEndBattle);
	TurnManager->OnPlayerInputEnabledChanged.AddUObject(this, &UCXCombatManagerSubsystem::HandlePlayerInputModeChanged);
}

void UCXCombatManagerSubsystem::HandlePlayerInputModeChanged(bool bAllowed)
{
	ACXCommandCharacter* CommandCharacter = WeakCommandCharacter.Get();

	if (!IsValid(CommandCharacter))
	{
		return;
	}

	CommandCharacter->SetCombatInputMode(bAllowed ? ECombatInputMode::Command : ECombatInputMode::Disabled);
}

void UCXCombatManagerSubsystem::HandleArenaReady(const TArray<TObjectPtr<class ACXCombatCharacterBase>>& Players, const TArray<TObjectPtr<class ACXCombatCharacterBase>>& Enemies)
{
	if (!IsValid(TurnManager))
	{
		return;
	}

	for (const auto& Character : Players)
	{
		if (ACXCombatCharacterPlayer* Player = Cast<ACXCombatCharacterPlayer>(Character.Get()))
		{
			WeakCommandCharacter->AddCommandCharacter(Player);
		}
	}
	
	StoredPlayerController->Possess(WeakCommandCharacter.Get());

	StoredPlayerController->SetViewTargetWithBlend(TestCombatArena.Get(), 0.f);

	TurnManager->StartBattle(Players, Enemies);
	
}

void UCXCombatManagerSubsystem::HandleArenaLevelReady()
{
	if (!bBattleRequested || !bLoadingCombat)
	{
		return;
	}

	bLoadingCombat = false;

	ULevelStreaming* Streaming = UGameplayStatics::GetStreamingLevel(this, CombatLevelName);

	ULevel* LoadedLevel = Streaming ? Streaming->GetLoadedLevel() : nullptr;

	if (!Streaming ||!Streaming->IsLevelVisible() || !LoadedLevel 
		|| !IsValid(ArenaManager) || !IsValid(StoredPlayerController))
	{
		UE_LOG(LogTemp, Warning, TEXT("전투 레벨 준비 실패"));
		RequestEndBattle();
		return;
	}

	// 로드한 전투 레벨 안에서만 아레나를 검색.
	ACXTestCombatArena* FoundArena = nullptr;
	ACXCommandCharacter* FoundCommandPawn = nullptr;

	for (AActor* Actor : LoadedLevel->Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		if (ACXTestCombatArena* Candidate = Cast<ACXTestCombatArena>(Actor))
		{
			if (FoundArena)
			{
				UE_LOG(LogTemp, Warning, TEXT("아레나가 두 개 이상입니다."));
				RequestEndBattle();
				return;
			}

			FoundArena = Candidate;
		}

		if (ACXCommandCharacter* Candidate = Cast<ACXCommandCharacter>(Actor))
		{
			if (FoundCommandPawn)
			{
				UE_LOG(LogTemp, Warning, TEXT("커맨드 Pawn이 두 개 이상입니다."));
				RequestEndBattle();
				return;
			}

			FoundCommandPawn = Candidate;
		}
	}

	if (!IsValid(FoundArena) || !IsValid(FoundCommandPawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("아레나 또는 커맨드 Pawn이 없습니다."));
		RequestEndBattle();
		return;
	}

	TestCombatArena = FoundArena;
	WeakCommandCharacter = FoundCommandPawn;

	// 레벨 표시와 아레나 검색이 끝난 뒤 생성 요청.
	if (!ArenaManager->PrepareArena(PendingSetupData, TestCombatArena.Get()))
	{
		UE_LOG(LogTemp, Warning, TEXT("전투원 생성 실패"));
		RequestEndBattle();
	}
}

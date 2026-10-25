// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/CXGameMode.h"

#include "Foundation/Scene/CXSceneDefinition.h"
#include "Foundation/Scene/CXSceneSubsystem.h"
#include "Level/CXLevelTransitionSubsystem.h"
#include "Level/CXLevelDefinition.h"

#include <GameFramework/PlayerStart.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGameMode)

void ACXGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	const UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this);
	const UCXLevelDefinition* LevelDefinition = LevelTransition ? LevelTransition->GetCurrentLevelDefinition() : nullptr;
	UCXSceneDefinition* InitialScene = LevelDefinition ? LevelDefinition->InitialScene.Get() : nullptr;

	UCXSceneSubsystem* SceneSubsystem = UCXSceneSubsystem::TryGet(this);
	if (nullptr == SceneSubsystem || nullptr == InitialScene)
	{
		UE_LOG(LogCXLevelTransition, Warning, TEXT("현재 레벨에 레벨 정의 또는 InitialScene이 없습니다. 첫 Scene 없이 시작합니다."));
		SpawnPendingPlayers();
		NotifyLevelReady();
		return;
	}

	// Scene Push 전에 구독 : 진입 직후(페이드 인 전) 스폰, 전환 완료 후 준비 완료 알림
	SceneEnteredHandle = SceneSubsystem->OnSceneEntered.AddUObject(this, &ThisClass::HandleInitialSceneEntered);
	SceneChangedHandle = SceneSubsystem->OnSceneChanged.AddUObject(this, &ThisClass::HandleInitialSceneReady);

	if (!SceneSubsystem->PushScene(InitialScene, FCXSceneContext()))
	{
		UnbindSceneDelegates();
		SpawnPendingPlayers();
		NotifyLevelReady();
	}
}

void ACXGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindSceneDelegates();

	Super::EndPlay(EndPlayReason);
}

void ACXGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// 아직 Scene 로딩 대기중 -> 필드 서브레벨이 없어서 PlayerStart를 찾을 수 없음
	if (bWaitingForInitialScene)
	{
		PendingStartPlayers.Add(NewPlayer);
		return;
	}

	// 그렇지 않다면 원래 흐름대로 Player Pawn을 스폰
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

void ACXGameMode::HandleInitialSceneEntered(UCXGameScene* InEnteredScene)
{
	if (UCXSceneSubsystem* SceneSubsystem = UCXSceneSubsystem::TryGet(this))
	{
		SceneSubsystem->OnSceneEntered.Remove(SceneEnteredHandle);
	}
	SceneEnteredHandle.Reset();

	// 지연 생성 Pawn이 있다면 생성
	SpawnPendingPlayers();
}

void ACXGameMode::HandleInitialSceneReady(UCXGameScene* InNewScene, UCXGameScene* InOldScene)
{
	// 첫 Scene 전환만 필요하므로 한 번 받으면 구독 해제
	if (UCXSceneSubsystem* SceneSubsystem = UCXSceneSubsystem::TryGet(this))
	{
		SceneSubsystem->OnSceneChanged.Remove(SceneChangedHandle);
	}
	SceneChangedHandle.Reset();

	// 레벨 준비가 완료되었다고 레벨 전이 서브시스템에 알림
	NotifyLevelReady();
}

void ACXGameMode::SpawnPendingPlayers()
{
	bWaitingForInitialScene = false;

	const UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this);
	const FName EntryTag = LevelTransition ? LevelTransition->GetPendingEntryTag() : NAME_None;

	// 생성 지연된 플레이어 컨트롤러를 확인
	for (const TWeakObjectPtr<APlayerController>& WeakPlayer : PendingStartPlayers)
	{
		APlayerController* Player = WeakPlayer.Get();
		if (nullptr == Player)
		{
			continue;
		}

		// IncomingName이 있으면 PlayerStartTag가 같은 PlayerStart를 찾음 (엔진 기본 동작), 없으면 ChoosePlayerStart
		AActor* StartSpot = FindPlayerStart(Player, EntryTag.IsNone() ? FString() : EntryTag.ToString());

		const APlayerStart* PlayerStart = Cast<APlayerStart>(StartSpot);
		if (!EntryTag.IsNone() && (nullptr == PlayerStart || PlayerStart->PlayerStartTag != EntryTag))
		{
			UE_LOG(LogCXLevelTransition, Warning, TEXT("진입 태그 '%s'에 해당하는 PlayerStart가 없습니다. 기본 위치에 스폰합니다."), *EntryTag.ToString());
		}

		if (StartSpot)
		{
			RestartPlayerAtPlayerStart(Player, StartSpot);
		}
		else
		{
			RestartPlayer(Player);
		}
	}

	PendingStartPlayers.Reset();
}

void ACXGameMode::UnbindSceneDelegates()
{
	if (UCXSceneSubsystem* SceneSubsystem = UCXSceneSubsystem::TryGet(this))
	{
		SceneSubsystem->OnSceneEntered.Remove(SceneEnteredHandle);
		SceneSubsystem->OnSceneChanged.Remove(SceneChangedHandle);
	}

	SceneEnteredHandle.Reset();
	SceneChangedHandle.Reset();
}

void ACXGameMode::NotifyLevelReady()
{
	if (UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this))
	{
		LevelTransition->NotifyLevelReady();
	}
}

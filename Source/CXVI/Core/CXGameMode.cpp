// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/CXGameMode.h"

#include "Foundation/Scene/CXSceneDefinition.h"
#include "Foundation/Scene/CXSceneSubsystem.h"
#include "Level/CXLevelTransitionSubsystem.h"
#include "Level/CXLevelDefinition.h"

#include <EngineUtils.h>
#include <GameFramework/PlayerStart.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGameMode)

AActor* ACXGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	const UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this);
	const FName EntryTag = LevelTransition ? LevelTransition->GetPendingEntryTag() : NAME_None;

	if (!EntryTag.IsNone())
	{
		// 첫 Scene 서브레벨이 이미 로딩된 상태이므로 서브레벨의 PlayerStart까지 검색됨
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			if (It->PlayerStartTag == EntryTag)
			{
				return *It;
			}
		}

		UE_LOG(LogCXLevelTransition, Warning, TEXT("진입 태그 '%s'에 해당하는 PlayerStart가 없습니다. 기본 위치에 스폰합니다."), *EntryTag.ToString());
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

void ACXGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// 첫 Scene이 컨트롤 Pawn을 정하면 Scene 진입 때 생성/빙의하므로 여기서는 따로 생성하지 않음
	// (StartSpot은 로그인 때 이미 정해져 있어서 Scene이 그대로 사용)
	const UCXSceneDefinition* InitialScene = GetInitialScene();
	if (InitialScene && InitialScene->PawnClass)
	{
		return;
	}

	// 게임 모드가 직접 컨트롤 Pawn을 생성
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

void ACXGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	UCXSceneDefinition* InitialScene = GetInitialScene();
	UCXSceneSubsystem* SceneSubsystem = UCXSceneSubsystem::TryGet(this);
	if (nullptr == SceneSubsystem || nullptr == InitialScene)
	{
		UE_LOG(LogCXLevelTransition, Warning, TEXT("현재 레벨에 레벨 정의 또는 InitialScene이 없습니다. 첫 Scene 없이 시작합니다."));
		NotifyLevelReady();
		return;
	}

	// 첫 Scene 진입이 끝나는 시점을 받기 위해 Push 전에 구독
	SceneChangedHandle = SceneSubsystem->OnSceneChanged.AddUObject(this, &ThisClass::HandleInitialSceneReady);

	// 서브레벨은 이미 Visible. 레벨 전환 구간의 화면은 레벨 전환이 맡으므로 화면 제어 없이 Push
	if (!SceneSubsystem->PushScene(InitialScene, FCXSceneContext(), /*bInUseFade*/ false))
	{
		UnbindSceneDelegates();
		NotifyLevelReady();
	}
}

void ACXGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindSceneDelegates();

	Super::EndPlay(EndPlayReason);
}

void ACXGameMode::HandleInitialSceneReady(UCXGameScene* InNewScene, UCXGameScene* InOldScene)
{
	// 첫 Scene 전환만 필요하므로 한 번 받으면 구독 해제
	UnbindSceneDelegates();

	NotifyLevelReady();
}

void ACXGameMode::UnbindSceneDelegates()
{
	if (SceneChangedHandle.IsValid())
	{
		if (UCXSceneSubsystem* SceneSubsystem = UCXSceneSubsystem::TryGet(this))
		{
			SceneSubsystem->OnSceneChanged.Remove(SceneChangedHandle);
		}

		SceneChangedHandle.Reset();
	}
}

void ACXGameMode::NotifyLevelReady()
{
	if (UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this))
	{
		LevelTransition->NotifyLevelReady();
	}
}

UCXSceneDefinition* ACXGameMode::GetInitialScene() const
{
	const UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this);
	if (nullptr == LevelTransition)
	{
		return nullptr;
	}

	const UCXLevelDefinition* LevelDefinition = LevelTransition->GetCurrentLevelDefinition();
	if (nullptr == LevelDefinition)
	{
		return nullptr;
	}

	return LevelDefinition->InitialScene.Get();
}

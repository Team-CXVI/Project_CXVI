// Fill out your copyright notice in the Description page of Project Settings.


#include "CXGameScene.h"

#include "CXSceneDefinition.h"
#include "CXSceneSubsystem.h"
#include "CXSceneSuspendable.h"

#include <EnhancedInputSubsystems.h>
#include <InputMappingContext.h>
#include <Engine/LevelStreaming.h>
#include <Engine/LocalPlayer.h>
#include <EngineUtils.h>
#include <GameFramework/PlayerController.h>
#include <Kismet/GameplayStatics.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGameScene)

UWorld* UCXGameScene::GetWorld() const
{
	// CDO는 소속 월드가 없기 때문에 예외처리
	if (HasAnyFlags(RF_ClassDefaultObject) || nullptr == GetOuter())
	{
		return nullptr;
	}

	return GetOuter()->GetWorld();
}

UCXSceneSubsystem* UCXGameScene::GetSceneSubsystem() const
{
	// 자신을 관리하는 SceneSubsystem을 반환
	return Cast<UCXSceneSubsystem>(GetOuter());
}

TArray<ULevel*> UCXGameScene::GetSceneLevels() const
{
	TArray<ULevel*> Levels;

	if (nullptr == Definition)
	{
		return Levels;
	}

	const UCXSceneSubsystem* Subsystem = GetSceneSubsystem();
	if (nullptr == Subsystem)
	{
		return Levels;
	}

	for (const TSoftObjectPtr<UWorld>& LevelRef : Definition->Sublevels)
	{
		if (const ULevelStreaming* Streaming = Subsystem->FindStreamingLevel(LevelRef))
		{
			if (ULevel* Level = Streaming->GetLoadedLevel())
			{
				Levels.Add(Level);
			}
		}
	}

	return Levels;
}

ULevel* UCXGameScene::GetPrimaryLevel() const
{
	if (nullptr == Definition || Definition->Sublevels.IsEmpty())
	{
		return nullptr;
	}

	const UCXSceneSubsystem* Subsystem = GetSceneSubsystem();
	if (nullptr == Subsystem)
	{
		return nullptr;
	}

	const ULevelStreaming* Streaming = Subsystem->FindStreamingLevel(Definition->Sublevels[0]);
	return Streaming ? Streaming->GetLoadedLevel() : nullptr;
}

void UCXGameScene::OnEnter(const FCXSceneContext& InContext)
{
	ApplyInputMappings();
	SetupSceneInput();
	K2_OnEnter(InContext);
}

void UCXGameScene::OnExit()
{
	K2_OnExit();
	ClearSceneInput();
	DestroySpawnedActors();
	RemoveInputMappings();
}

void UCXGameScene::OnSuspend()
{
	K2_OnSuspend();
	ClearSceneInput();
	NotifySuspendables(true);
	RemoveInputMappings();
}

void UCXGameScene::OnResume(const FCXSceneContext& InContext)
{
	ApplyInputMappings();
	NotifySuspendables(false);
	SetupSceneInput();
	K2_OnResume(InContext);
}

UEnhancedInputComponent* UCXGameScene::GetPlayerInputComponent() const
{
	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	return PC ? Cast<UEnhancedInputComponent>(PC->InputComponent) : nullptr;
}

void UCXGameScene::ApplyInputMappings() const
{
	if(nullptr == Definition)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const ULocalPlayer* LocalPlayer = World ? World->GetFirstLocalPlayerFromController() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (nullptr == InputSubsystem)
	{
		return;
	}

	// 키 리바인딩(EnhancedInputUserSettings)에 IMC등록
	FModifyContextOptions Options;
	Options.bNotifyUserSettings = true;

	for (const FCXSceneInputMapping& Mapping : Definition->IMCs)
	{
		UInputMappingContext* IMC = Mapping.IMC.LoadSynchronous();
		if (nullptr == IMC)
		{
			continue;
		}

		InputSubsystem->AddMappingContext(IMC, Mapping.Priority, Options);
	}
}

void UCXGameScene::RemoveInputMappings() const
{
	if (nullptr == Definition)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const ULocalPlayer* LocalPlayer = World ? World->GetFirstLocalPlayerFromController() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (nullptr == InputSubsystem)
	{
		return;
	}

	FModifyContextOptions Options;
	Options.bNotifyUserSettings = true;

	for (const FCXSceneInputMapping& Mapping : Definition->IMCs)
	{
		UInputMappingContext* IMC = Mapping.IMC.LoadSynchronous();
		if (nullptr == IMC)
		{
			continue;
		}

		InputSubsystem->RemoveMappingContext(IMC, Options);
	}
}

void UCXGameScene::ClearSceneInput()
{
	if (UEnhancedInputComponent* Input = GetPlayerInputComponent())
	{
		for (const uint32 Handle : InputBindingHandles)
		{
			Input->RemoveBindingByHandle(Handle);
		}
	}

	InputBindingHandles.Reset();
}

void UCXGameScene::NotifySuspendables(bool bInSuspended) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TSet<const ULevel*> TargetLevels;
	TargetLevels.Emplace(World->PersistentLevel);
	for (const ULevel* Level : GetSceneLevels())
	{
		TargetLevels.Emplace(Level);
	}

	// TActorIterator는 기본적으로 Visible 레벨만 순회함
	// -> Suspend는 Hide 전에, Resume은 Show 후에 호출되도록 Subsystem의 단계 순서로 보장
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (false == TargetLevels.Contains(Actor->GetLevel()))
		{
			continue;
		}

		if (false == Actor->Implements<UCXSceneSuspendable>())
		{
			continue;
		}

		if (bInSuspended)
		{
			ICXSceneSuspendable::Execute_OnSceneSuspended(Actor);
		}
		else
		{
			ICXSceneSuspendable::Execute_OnSceneResumed(Actor);
		}
	}
}

void UCXGameScene::DestroySpawnedActors()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}

	SpawnedActors.Reset();
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "CXSceneSubsystem.h"

#include "CXSceneDefinition.h"
#include "CXGameScene.h"

#include <Kismet/GameplayStatics.h>
#include <Camera/PlayerCameraManager.h>
#include <TimerManager.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXSceneSubsystem)

DEFINE_LOG_CATEGORY(LogCXScene);

UCXSceneSubsystem& UCXSceneSubsystem::Get(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	check(World);

	UCXSceneSubsystem* Subsystem = World->GetSubsystem<UCXSceneSubsystem>();
	check(Subsystem);

	return *Subsystem;
}

UCXSceneSubsystem* UCXSceneSubsystem::TryGet(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	return World ? World->GetSubsystem<UCXSceneSubsystem>() : nullptr;
}

bool UCXSceneSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UCXSceneSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LevelPollHandle);
		World->GetTimerManager().ClearTimer(FadeHandle);
	}

	// 월드 종료 중에는 OnExit를 호출하지 않음 (이미 다른 곳에서 정리 중)
	Transition = FCXSceneTransition();
	bIsTransitioning = false;
	PendingLevels.Reset();
	ManagedLevels.Reset();
	ResidentDefinitions.Reset();
	SceneStack.Reset();

	Super::Deinitialize();
}

bool UCXSceneSubsystem::PushScene(UCXSceneDefinition* InDefinition, const FCXSceneContext& InContext)
{
	if (!CanStartTransition(TEXT("PushScene")))
	{
		return false;
	}

	if (nullptr == InDefinition || nullptr == InDefinition->SceneClass)
	{
		UE_LOG(LogCXScene, Error, TEXT("PushScene 실패: Definition 또는 SceneClass가 없습니다."));
		return false;
	}

	Transition = FCXSceneTransition();
	Transition.IncomingDefinition = InDefinition;
	Transition.OutgoingScene = GetCurrentScene();
	Transition.Context = InContext;
	Transition.FadeOutDuration = InDefinition->FadeOutDuration;
	Transition.FadeInDuration = InDefinition->FadeInDuration;
	Transition.FadeColor = InDefinition->FadeColor;
	Transition.Steps = {
		ECXSceneTransitionStep::FadeOut,
		ECXSceneTransitionStep::ShowIncomingLevels,
		ECXSceneTransitionStep::SuspendOutgoing,
		ECXSceneTransitionStep::PushIncoming,
		ECXSceneTransitionStep::ResolveLevels,
		ECXSceneTransitionStep::EnterIncoming,
		ECXSceneTransitionStep::FadeIn
	};

	StartTransition();
	return true;
}

bool UCXSceneSubsystem::PopScene(const FCXSceneContext& InResultContext)
{
	if (false == CanStartTransition(TEXT("PopScene")))
	{
		return false;
	}

	if (SceneStack.Num() < 2)
	{
		UE_LOG(LogCXScene, Warning, TEXT("PopScene 무시: 돌아갈 씬이 없습니다."));
		return false;
	}

	UCXGameScene* Outgoing = SceneStack.Last();
	UCXGameScene* Incoming = SceneStack[SceneStack.Num() - 2];

	Transition = FCXSceneTransition();
	Transition.OutgoingScene = Outgoing;
	Transition.IncomingScene = Incoming;
	Transition.Context = InResultContext;
	Transition.FadeOutDuration = Outgoing->Definition->FadeOutDuration;
	Transition.FadeInDuration = Incoming->Definition->FadeInDuration;
	Transition.FadeColor = Outgoing->Definition->FadeColor;
	Transition.Steps = {
		ECXSceneTransitionStep::FadeOut,
		ECXSceneTransitionStep::ExitOutgoing,
		ECXSceneTransitionStep::PopOutgoing,
		ECXSceneTransitionStep::ResolveLevels,
		ECXSceneTransitionStep::ResumeIncoming,
		ECXSceneTransitionStep::FadeIn
	};

	StartTransition();
	return true;
}

void UCXSceneSubsystem::PreloadScene(UCXSceneDefinition* InDefinition)
{
	if (nullptr == InDefinition)
	{
		return;
	}

	ResidentDefinitions.Add(InDefinition);

	// 레벨 전환 중이라면 ResolveLevels 단계에서 처리됨
	if (false == bIsTransitioning)
	{
		ApplyLevelTargets(BuildResolvedTargets(), /*bInWait*/ false);
	}
}

void UCXSceneSubsystem::ReleaseScene(UCXSceneDefinition* InDefinition)
{
	if (!InDefinition)
	{
		return;
	}

	ResidentDefinitions.Remove(InDefinition);

	if (!bIsTransitioning)
	{
		ApplyLevelTargets(BuildResolvedTargets(), /*bInWait*/ false);
	}
}

UCXGameScene* UCXSceneSubsystem::GetCurrentScene() const
{
	return SceneStack.IsEmpty() ? nullptr : SceneStack.Last().Get();;
}

ULevelStreaming* UCXSceneSubsystem::FindStreamingLevel(const TSoftObjectPtr<UWorld>& InLevel) const
{
	if (InLevel.IsNull())
	{
		return nullptr;
	}

	// GetStreamingLevel이 PIE 패키지 이름 접두사(UEDPIE_N_)를 알아서 처리함
	return UGameplayStatics::GetStreamingLevel(GetWorld(), FName(*InLevel.GetLongPackageName()));
}

bool UCXSceneSubsystem::CanStartTransition(const TCHAR* InRequestName) const
{
	if (bIsTransitioning)
	{
		UE_LOG(LogCXScene, Warning, TEXT("%s 무시: 이미 전환 중입니다."), InRequestName);
		return false;
	}

	return true;
}

void UCXSceneSubsystem::StartTransition()
{
	bIsTransitioning = true;
	RunNextStep();
}

void UCXSceneSubsystem::RunNextStep()
{
	while (Transition.StepIndex < Transition.Steps.Num())
	{
		const ECXSceneTransitionStep Step = Transition.Steps[Transition.StepIndex++];
		UE_LOG(LogCXScene, Log, TEXT("Transition step: %s"), StepToString(Step));

		if (ExecuteStep(Step))
		{
			// 비동기 대기 중. 완료되면 타이머가 RunNextStep을 다시 호출
			return;
		}
	}

	FinishTransition();
}

bool UCXSceneSubsystem::ExecuteStep(ECXSceneTransitionStep InStep)
{
	FCXSceneTransition& T = Transition;

	switch (InStep)
	{

		case ECXSceneTransitionStep::FadeOut:
		{
			return StartFade(true, T.FadeOutDuration, T.FadeColor);
		}

		case ECXSceneTransitionStep::ShowIncomingLevels:
		{
			TMap<FSoftObjectPath, ECXSublevelTarget> Targets;
			for (const TSoftObjectPtr<UWorld>& Level : T.IncomingDefinition->Sublevels)
			{
				Targets.Add(Level.ToSoftObjectPath(), ECXSublevelTarget::Visible);
			}
			return ApplyLevelTargets(Targets, /*bInWait*/ true);
		}

		case ECXSceneTransitionStep::SuspendOutgoing:
		{
			if (T.OutgoingScene)
			{
				T.OutgoingScene->OnSuspend();
			}
			return false;
		}

		case ECXSceneTransitionStep::PushIncoming:
		{
			T.IncomingScene = NewObject<UCXGameScene>(this, T.IncomingDefinition->SceneClass);
			T.IncomingScene->Definition = T.IncomingDefinition;
			SceneStack.Push(T.IncomingScene);

			for (UCXSceneDefinition* Preload : T.IncomingDefinition->PreloadScenes)
			{
				if (Preload)
				{
					ResidentDefinitions.Add(Preload);
				}
			}
			return false;
		}

		case ECXSceneTransitionStep::ExitOutgoing:
		{
			T.OutgoingScene->OnExit();
			return false;
		}

		case ECXSceneTransitionStep::PopOutgoing:
		{
			SceneStack.Pop();

			UCXSceneDefinition* Definition = T.OutgoingScene->Definition;
			if (Definition->ExitPolicy == ECXSceneExitPolicy::KeepLoaded)
			{
				ResidentDefinitions.Add(Definition);
			}
			else
			{
				ResidentDefinitions.Remove(Definition);
			}
			return false;
		}

		case ECXSceneTransitionStep::ResolveLevels:
		{
			return ApplyLevelTargets(BuildResolvedTargets(), /*bInWait*/ true);
		}

		case ECXSceneTransitionStep::EnterIncoming:
		{
			T.IncomingScene->OnEnter(T.Context);
			return false;
		}

		case ECXSceneTransitionStep::ResumeIncoming:
		{
			T.IncomingScene->OnResume(T.Context);
			return false;
		}

		case ECXSceneTransitionStep::FadeIn:
		{
			return StartFade(false, T.FadeInDuration, T.FadeColor);
		}
	}

	return false;
}

void UCXSceneSubsystem::FinishTransition()
{
	UCXGameScene* NewScene = Transition.IncomingScene;
	UCXGameScene* OldScene = Transition.OutgoingScene;

	Transition = FCXSceneTransition();
	bIsTransitioning = false;

	UE_LOG(LogCXScene, Log, TEXT("Scene changed: %s -> %s"),
		OldScene ? *OldScene->GetClass()->GetName() : TEXT("None"),
		NewScene ? *NewScene->GetClass()->GetName() : TEXT("None"));

	//  GC 되기 전에 델리게이트 알림
	OnSceneChanged.Broadcast(NewScene, OldScene);
}

const TCHAR* UCXSceneSubsystem::StepToString(ECXSceneTransitionStep InStep)
{
	switch (InStep)
	{
	case ECXSceneTransitionStep::FadeOut:            return TEXT("FadeOut");
	case ECXSceneTransitionStep::ShowIncomingLevels: return TEXT("ShowIncomingLevels");
	case ECXSceneTransitionStep::SuspendOutgoing:    return TEXT("SuspendOutgoing");
	case ECXSceneTransitionStep::PushIncoming:       return TEXT("PushIncoming");
	case ECXSceneTransitionStep::ExitOutgoing:       return TEXT("ExitOutgoing");
	case ECXSceneTransitionStep::PopOutgoing:        return TEXT("PopOutgoing");
	case ECXSceneTransitionStep::ResolveLevels:      return TEXT("ResolveLevels");
	case ECXSceneTransitionStep::EnterIncoming:      return TEXT("EnterIncoming");
	case ECXSceneTransitionStep::ResumeIncoming:     return TEXT("ResumeIncoming");
	case ECXSceneTransitionStep::FadeIn:             return TEXT("FadeIn");
	}
	return TEXT("Unknown");
}

TMap<FSoftObjectPath, ECXSublevelTarget> UCXSceneSubsystem::BuildResolvedTargets() const
{
	TMap<FSoftObjectPath, ECXSublevelTarget> Targets;

	// 기본값 -> 아무도 참조하지 않으면 Unloaded
	for (const FSoftObjectPath& Path : ManagedLevels)
	{
		Targets.Add(Path, ECXSublevelTarget::Unloaded);
	}

	auto Raise = [&Targets](const UCXSceneDefinition* InDefinition, ECXSublevelTarget InTarget)
		{
			if (nullptr == InDefinition)
			{
				return;
			}

			for (const TSoftObjectPtr<UWorld>& Level : InDefinition->Sublevels)
			{
				ECXSublevelTarget& Current = Targets.FindOrAdd(Level.ToSoftObjectPath(), ECXSublevelTarget::Unloaded);
				if (static_cast<uint8>(InTarget) > static_cast<uint8>(Current))
				{
					Current = InTarget;
				}
			}
		};

	for (const UCXSceneDefinition* Definition : ResidentDefinitions)
	{
		Raise(Definition, ECXSublevelTarget::Hidden);
	}

	for (int32 Index = 0; Index < SceneStack.Num(); ++Index)
	{
		const bool bIsTop = (Index == SceneStack.Num() - 1);
		Raise(SceneStack[Index]->Definition, bIsTop ? ECXSublevelTarget::Visible : ECXSublevelTarget::Hidden);
	}

	return Targets;
}

bool UCXSceneSubsystem::ApplyLevelTargets(const TMap<FSoftObjectPath, ECXSublevelTarget>& InTargets, bool bInWait)
{
	PendingLevels.Reset();

	for (const TPair<FSoftObjectPath, ECXSublevelTarget>& Pair : InTargets)
	{
		ManagedLevels.Add(Pair.Key);

		ULevelStreaming* Streaming = FindStreamingLevel(TSoftObjectPtr<UWorld>(Pair.Key));
		if (nullptr == Streaming)
		{
			UE_LOG(LogCXScene, Error, TEXT("서브레벨 '%s'가 퍼시스턴트 맵에 등록되어 있지 않습니다."), *Pair.Key.ToString());
			continue;
		}

		Streaming->SetShouldBeLoaded(Pair.Value != ECXSublevelTarget::Unloaded);
		Streaming->SetShouldBeVisible(Pair.Value == ECXSublevelTarget::Visible);

		if (false == IsInTargetState(Streaming, Pair.Value))
		{
			PendingLevels.Emplace(Streaming, Pair.Value);
		}
	}

	if (false == bInWait || PendingLevels.IsEmpty())
	{
		PendingLevels.Reset();
		return false;
	}

	PendingStartTime = FPlatformTime::Seconds();
	GetWorld()->GetTimerManager().SetTimer(LevelPollHandle, this, &UCXSceneSubsystem::PollLevelTargets, LevelPollInterval, true);

	return true;
}

void UCXSceneSubsystem::PollLevelTargets()
{
	bool bAllDone = true;

	for (const TPair<TWeakObjectPtr<ULevelStreaming>, ECXSublevelTarget>& Pending : PendingLevels)
	{
		const ULevelStreaming* Streaming = Pending.Key.Get();
		if (nullptr == Streaming)
		{
			continue;
		}

		if (Streaming->GetLevelStreamingState() == ELevelStreamingState::FailedToLoad)
		{
			UE_LOG(LogCXScene, Error, TEXT("서브레벨 로드 실패: %s"), *Streaming->GetWorldAssetPackageName());
			continue;
		}

		if (!IsInTargetState(Streaming, Pending.Value))
		{
			bAllDone = false;
			break;
		}
	}

	const bool bTimedOut = (FPlatformTime::Seconds() - PendingStartTime) > LevelTimeoutSeconds;
	if (!bAllDone && !bTimedOut)
	{
		return;
	}

	if (!bAllDone)
	{
		UE_LOG(LogCXScene, Error, TEXT("서브레벨 상태 변경 시간 초과 (%.0f초). 전환을 계속 진행합니다."), LevelTimeoutSeconds);
	}

	GetWorld()->GetTimerManager().ClearTimer(LevelPollHandle);
	PendingLevels.Reset();
	RunNextStep();
}

bool UCXSceneSubsystem::IsInTargetState(const ULevelStreaming* InStreaming, ECXSublevelTarget InTarget)
{
	const ELevelStreamingState State = InStreaming->GetLevelStreamingState();

	switch (InTarget)
	{
	case ECXSublevelTarget::Visible:
		return State == ELevelStreamingState::LoadedVisible;

	case ECXSublevelTarget::Hidden:
		return State == ELevelStreamingState::LoadedNotVisible;

	case ECXSublevelTarget::Unloaded:
		return State == ELevelStreamingState::Unloaded || State == ELevelStreamingState::Removed;
	}

	return false;
}

bool UCXSceneSubsystem::StartFade(bool bInFadeOut, float InDuration, const FLinearColor& InColor)
{
	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (nullptr == CameraManager || InDuration <= 0.f)
	{
		if (CameraManager)
		{
			if (bInFadeOut)
			{
				CameraManager->SetManualCameraFade(1.f, InColor, /*bInFadeAudio*/ false);
			}
			else
			{
				CameraManager->StopCameraFade();
			}
		}

		return false;
	}

	const float FromAlpha = bInFadeOut ? 0.f : 1.f;
	const float ToAlpha = bInFadeOut ? 1.f : 0.f;

	// 페이드 아웃은 끝난 뒤에도 화면을 가린 상태로 유지(bHoldWhenFinished)
	CameraManager->StartCameraFade(FromAlpha, ToAlpha, InDuration, InColor, /*bShouldFadeAudio*/ true, /*bHoldWhenFinished*/ bInFadeOut);
	GetWorld()->GetTimerManager().SetTimer(FadeHandle, this, &UCXSceneSubsystem::RunNextStep, InDuration, false);

	return true;
}

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CXSceneTypes.h"
#include <Engine/StreamableManager.h>

#include "CXSceneSubsystem.generated.h"

class ULevelStreaming;

DECLARE_LOG_CATEGORY_EXTERN(LogCXScene, Log, All);

// Scene의 OnEnter 직후, Fade In 전에 호출(레벨은 Visible인 상태)
DECLARE_MULTICAST_DELEGATE_OneParam(FCXOnSceneEntered, UCXGameScene* /*EnteredScene*/);

DECLARE_MULTICAST_DELEGATE_TwoParams(FCXOnSceneChanged, UCXGameScene* /*NewScene*/, UCXGameScene* /*OldScene*/);


// 값이 클수록 우선 순위가 높음(Scene에 중복으로 걸친 Sublevel이 있고 그중 Visible인 Scene이 있다면 거길 따라감)
enum class ECXSublevelTarget : uint8
{
	Unloaded = 0,
	Hidden = 1,
	Visible = 2
};

enum class ECXSceneTransitionStep : uint8
{
	FadeOut,
	LoadAssets,					// Context.AssetsToLoad 비동기 로드 (화면이 가려진 뒤)
	ShowIncomingLevels,			// 새 Scene 레벨 Visible
	SuspendOutgoing,
	PushIncoming,				// 새 Scene Object 생성 + 스택 Push + Preload 등록
	ExitOutgoing,
	PopOutgoing,				// 스택 Pop + Exit 정책에 따라 Resident 갱신
	ResolveLevels,				// 전체 레벨 상태 재계산
	EnterIncoming,
	ResumeIncoming,
	FadeIn
};

/** Scene 전환 상태 관리 구조체 */
USTRUCT()
struct FCXSceneTransition
{
	GENERATED_BODY()

public:
	// New Scene의 Definition
	UPROPERTY()
	TObjectPtr<UCXSceneDefinition> IncomingDefinition;

	UPROPERTY()
	TObjectPtr<UCXGameScene> IncomingScene;

	// Old Scene
	UPROPERTY()
	TObjectPtr<UCXGameScene> OutgoingScene;

	UPROPERTY()
	FCXSceneContext Context;

	TArray<ECXSceneTransitionStep> Steps;

	int32 StepIndex = 0;

	float FadeOutDuration = 0.f;

	float FadeInDuration = 0.f;

	FLinearColor FadeColor = FLinearColor::Black;

	// LoadAssets 단계의 로드 핸들. 전환이 끝나면(구조체 초기화) 해제
	TSharedPtr<FStreamableHandle> AssetLoadHandle;
};

/**
 * 게임 맵 안의 Scene 스택 관리
 * - 전환은 단계 목록(Steps)을 순서대로 실행. 대기가 필요한 단계에서 멈췄다가 완료 시 이어서 진행
 * - 레벨 상태는 "스택 + Resident 정의"를 기준으로 매번 계산 (Scene 간 레벨 공유도 가능)
 * - 전환은 한 번에 하나만 진행
 * - Context.AssetsToLoad가 있으면 FadeOut 다음 LoadAssets 단계에서 비동기 로드 후 진행 (화면이 보이는 동안 동기 로드하지 않도록)
 */
UCLASS()
class CXVI_API UCXSceneSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	static UCXSceneSubsystem& Get(const UObject* WorldContextObject);

	static UCXSceneSubsystem* TryGet(const UObject* WorldContextObject);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	virtual void Deinitialize() override;

public:
	// bInUseFade = false : 화면 페이드 제어 없이 Push (레벨 전환 구간처럼 다른 쪽이 화면을 맡고 있을 때)
	UFUNCTION(BlueprintCallable, Category = "CXVI|Scene")
	bool PushScene(UCXSceneDefinition* InDefinition, const FCXSceneContext& InContext, bool bInUseFade = true);
	
	UFUNCTION(BlueprintCallable, Category = "CXVI|Scene")
	bool PopScene(const FCXSceneContext& InResultContext);

	// Scene 레벨을 Hidden 상태로 미리 로드해서 유지 (대기하지 않음)
	UFUNCTION(BlueprintCallable, Category = "CXVI|Scene")
	void PreloadScene(UCXSceneDefinition* InDefinition);

	// Preload / KeepLoaded로 유지 중인 Scene 레벨을 해제 (다른 Scene이 참조하지 않으면 언로드)
	UFUNCTION(BlueprintCallable, Category = "CXVI|Scene")
	void ReleaseScene(UCXSceneDefinition* InDefinition);

	UFUNCTION(BlueprintPure, Category = "CXVI|Scene")
	UCXGameScene* GetCurrentScene() const;

	UFUNCTION(BlueprintPure, Category = "CXVI|Scene")
	bool IsTransitioning() const { return bIsTransitioning; }

	ULevelStreaming* FindStreamingLevel(const TSoftObjectPtr<UWorld>& InLevel) const;

	// Scene의 서브레벨을 동기로 로드(레벨 전이에 맞춰서 로딩)하고 Visible로 만듦
	void LoadSceneLevelsImmediately(const UCXSceneDefinition* InDefinition);

private:
	bool CanStartTransition(const TCHAR* InRequestName) const;

	void StartTransition();

	void RunNextStep();

	// 반환값: true = 비동기 대기 시작 (완료 시 RunNextStep 재호출), false = 즉시 완료
	bool ExecuteStep(ECXSceneTransitionStep InStep);

	void FinishTransition();

	static const TCHAR* StepToString(ECXSceneTransitionStep InStep);

private:
	TMap<FSoftObjectPath, ECXSublevelTarget> BuildResolvedTargets() const;

	// 반환값: true = 대기 시작
	bool ApplyLevelTargets(const TMap<FSoftObjectPath, ECXSublevelTarget>& InTargets, bool bInWait);

	void PollLevelTargets();

	static bool IsInTargetState(const ULevelStreaming* InStreaming, ECXSublevelTarget InTarget);

private:
	// 반환값: true = 대기 시작
	bool StartFade(bool bInFadeOut, float InDuration, const FLinearColor& InColor);

	// 반환값: true = 대기 시작 (완료 시 HandleAssetsLoaded → RunNextStep), false = 로드할 것이 없거나 이미 로드됨
	bool StartAssetLoad(const TArray<FSoftObjectPath>& InAssets);

	void HandleAssetsLoaded();

public:
	FCXOnSceneEntered OnSceneEntered;

	FCXOnSceneChanged OnSceneChanged;

private:
	// Scene을 관리하는 스택. 가장 위에 있는 Scene이 Visible
	UPROPERTY()
	TArray<TObjectPtr<UCXGameScene>> SceneStack;

	// Scene 스택에는 없어도 Hidden으로 유지할 정의 (Preload + ECXSceneExitPolicy::KeepLoaded)
	UPROPERTY()
	TSet<TObjectPtr<UCXSceneDefinition>> ResidentDefinitions;

	UPROPERTY()
	FCXSceneTransition Transition;
	
	bool bIsTransitioning = false;

	// 한 번이라도 상태를 변경한 레벨 (아무도 참조하지 않게 되면 Unloaded로 돌림)
	TSet<FSoftObjectPath> ManagedLevels;

	TArray<TPair<TWeakObjectPtr<ULevelStreaming>, ECXSublevelTarget>> PendingLevels;

	double PendingStartTime = 0.0;

	FTimerHandle LevelPollHandle;

	FTimerHandle FadeHandle;

	// LoadAssets 단계에서 완료 콜백을 기다리는 중
	// 요청 안에서 바로 완료 콜백이 불리는 경우(이미 로드됨)는 false 상태라 무시되고, 요청 쪽에서 즉시 진행
	bool bIsWaitingAssets = false;

	// 로드 시간 측정용 (로그)
	double AssetLoadStartTime = 0.0;

	static constexpr float LevelPollInterval = 0.02f;

	static constexpr double LevelTimeoutSeconds = 30.0;
};

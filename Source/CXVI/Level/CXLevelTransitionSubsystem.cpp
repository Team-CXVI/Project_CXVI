// Fill out your copyright notice in the Description page of Project Settings.


#include "Level/CXLevelTransitionSubsystem.h"

#include "CXLevelTransitionSettings.h"
#include "CXLevelDefinition.h"
#include "Foundation/Scene/CXSceneSubsystem.h"

#include <Engine/AssetManager.h>
#include <Kismet/GameplayStatics.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXLevelTransitionSubsystem)

DEFINE_LOG_CATEGORY(LogCXLevelTransition);

UCXLevelTransitionSubsystem& UCXLevelTransitionSubsystem::Get(const UObject* WorldContextObject)
{
	UCXLevelTransitionSubsystem* Subsystem = TryGet(WorldContextObject);
	check(Subsystem);

	return *Subsystem;
}

UCXLevelTransitionSubsystem* UCXLevelTransitionSubsystem::TryGet(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UCXLevelTransitionSubsystem>() : nullptr;
}

void UCXLevelTransitionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 레벨 로드 실패 시 전환 상태에 갇히지 않도록 초기화
	TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);

	// 액터 초기화 직후(플레이어 스폰 전)에 현재 레벨 정의 갱신 + 첫 Scene 서브레벨 동기 로드 처리
	WorldActorsInitializedHandle = FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &ThisClass::HandleWorldActorsInitialized);

#if !UE_BUILD_SHIPPING
	ValidateSettings();
#endif
}

void UCXLevelTransitionSubsystem::Deinitialize()
{
	// 델리게이트 해제
	if (GEngine)
	{
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	}
	TravelFailureHandle.Reset();

	FWorldDelegates::OnWorldInitializedActors.Remove(WorldActorsInitializedHandle);
	WorldActorsInitializedHandle.Reset();

	GetGameInstance()->GetTimerManager().ClearTimer(FadeHandle);
	ResetTransition();
	CurrentDefinition = nullptr;

	Super::Deinitialize();
}

bool UCXLevelTransitionSubsystem::TravelToTitle()
{
	const FPrimaryAssetId& TitleLevelId = GetDefault<UCXLevelTransitionSettings>()->TitleLevel;

	UCXLevelDefinition* Definition = LoadLevelDefinition(TitleLevelId);
	if (nullptr == Definition)
	{
		UE_LOG(LogCXLevelTransition, Error, TEXT("TravelToTitle 실패: 타이틀 레벨 정의를 찾을 수 없습니다. [%s] (Project Settings > Game > CXVI Level Transition)"),
			*TitleLevelId.ToString());
		return false;
	}

	return BeginTransition(Definition, NAME_None);
}

bool UCXLevelTransitionSubsystem::TravelToLevel(FPrimaryAssetId InLevelId, FName InEntryTag)
{
	// 타이틀 이동은 호출하는 쪽에서 명시적으로 TravelToTitle을 사용
	if (InLevelId == GetDefault<UCXLevelTransitionSettings>()->TitleLevel)
	{
		UE_LOG(LogCXLevelTransition, Warning, TEXT("TravelToLevel 무시: 타이틀 레벨은 TravelToTitle을 사용하세요."));
		return false;
	}

	UCXLevelDefinition* Definition = LoadLevelDefinition(InLevelId);
	if (nullptr == Definition)
	{
		UE_LOG(LogCXLevelTransition, Error, TEXT("TravelToLevel 실패: 레벨 정의를 찾을 수 없습니다. [%s]"), *InLevelId.ToString());
		return false;
	}

	return BeginTransition(Definition, InEntryTag);
}

bool UCXLevelTransitionSubsystem::QuitGame()
{
	if (bIsTransitioning)
	{
		UE_LOG(LogCXLevelTransition, Warning, TEXT("QuitGame Failed: 레벨 전환 중입니다."));
		return false;
	}

	UWorld* World = GetGameInstance()->GetWorld();
	UKismetSystemLibrary::QuitGame(World, UGameplayStatics::GetPlayerController(World, 0), EQuitPreference::Quit, false);
	return true;
}

void UCXLevelTransitionSubsystem::ApplyScreenCover(APlayerCameraManager* InCameraManager) const
{
	if (bScreenCovered && InCameraManager)
	{
		InCameraManager->SetManualCameraFade(1.f, CoverColor, /*bInFadeAudio*/ false);
	}
}

void UCXLevelTransitionSubsystem::NotifyLevelReady()
{
	// 레벨 전이 없이 호출된 이벤트 (부트 맵, 에디터에서 바로 PIE) 또는 이미 카메라 노출 중
	if (!bIsTransitioning || bIsRevealing)
	{
		return;
	}

	bIsRevealing = true;
	bScreenCovered = false;

	const float FadeInDuration = CurrentDefinition ? CurrentDefinition->FadeInDuration : 0.f;
	const FTimerDelegate OnRevealed = FTimerDelegate::CreateUObject(this, &ThisClass::FinishTransition);

	if (!StartCameraFade(/*bInFadeOut*/ false, FadeInDuration, CoverColor, OnRevealed))
	{
		FinishTransition();
	}
}

bool UCXLevelTransitionSubsystem::BeginTransition(UCXLevelDefinition* InDefinition, FName InEntryTag)
{
	if (bIsTransitioning)
	{
		UE_LOG(LogCXLevelTransition, Warning, TEXT("레벨 이동 무시: 이미 레벨 전환 중입니다."));
		return false;
	}

	if (nullptr == InDefinition || InDefinition->Level.IsNull())
	{
		UE_LOG(LogCXLevelTransition, Error, TEXT("레벨 이동 실패: 레벨 정의의 Level이 비어 있습니다. [%s]"), *GetNameSafe(InDefinition));
		return false;
	}

	// Scene 전환 도중에는 레벨 전이가 발생하지 않음
	const UWorld* World = GetGameInstance()->GetWorld();
	const UCXSceneSubsystem* SceneSubsystem = World ? World->GetSubsystem<UCXSceneSubsystem>() : nullptr;
	if (SceneSubsystem && SceneSubsystem->IsTransitioning())
	{
		UE_LOG(LogCXLevelTransition, Warning, TEXT("레벨 이동 무시: Scene 전환 중입니다."));
		return false;
	}

	bIsTransitioning = true;
	PendingDefinition = InDefinition;
	PendingEntryTag = InEntryTag;

	// Fade Out 시작 : 이후 새 월드의 CameraManager에도 이 상태가 다시 적용됨
	bScreenCovered = true;
	CoverColor = InDefinition->FadeColor;

	UE_LOG(LogCXLevelTransition, Log, TEXT("Level transition started: %s -> %s (Entry: %s)"),
		*GetNameSafe(CurrentDefinition), *InDefinition->GetName(), *InEntryTag.ToString());

	OnTransitionStarted.Broadcast(InDefinition);

	// 페이드 아웃은 목표 레벨 정의의 값을 사용. 끝나면 레벨 열기
	const FTimerDelegate OnCovered = FTimerDelegate::CreateUObject(this, &ThisClass::OpenPendingLevel);
	if (!StartCameraFade(/*bInFadeOut*/ true, InDefinition->FadeOutDuration, CoverColor, OnCovered))
	{
		OpenPendingLevel();
	}

	return true;
}

UCXLevelDefinition* UCXLevelTransitionSubsystem::LoadLevelDefinition(const FPrimaryAssetId& InLevelId) const
{
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (nullptr == AssetManager || !InLevelId.IsValid())
	{
		return nullptr;
	}

	const FSoftObjectPath AssetPath = AssetManager->GetPrimaryAssetPath(InLevelId);
	return Cast<UCXLevelDefinition>(AssetPath.TryLoad());
}

UCXLevelDefinition* UCXLevelTransitionSubsystem::FindLevelDefinitionForWorld(const UWorld* InWorld) const
{
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (nullptr == AssetManager || nullptr == InWorld)
	{
		return nullptr;
	}

	// PrimaryAssetType 이름 => C++ 클래스 이름
	const FPrimaryAssetType LevelType(UCXLevelDefinition::StaticClass()->GetFName());

	TArray<FPrimaryAssetId> LevelIds;
	AssetManager->GetPrimaryAssetIdList(LevelType, LevelIds);

	// PIE에서는 패키지 이름에 UEDPIE_N_ 접두사가 붙으므로 제거 후 비교
	const FString WorldPackageName = UWorld::RemovePIEPrefix(InWorld->GetPackage()->GetName());

	for (const FPrimaryAssetId& LevelId : LevelIds)
	{
		UCXLevelDefinition* Definition = LoadLevelDefinition(LevelId);
		if (Definition && Definition->Level.GetLongPackageName() == WorldPackageName)
		{
			return Definition;
		}
	}
	return nullptr;
}

void UCXLevelTransitionSubsystem::HandleWorldActorsInitialized(const UWorld::FActorsInitializedParams& InParams)
{
	UWorld* World = InParams.World;
	if (nullptr == World || World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	// 레벨 전이 중이 었다면 목적 레벨 정의, 그렇지 않다면 월드에 맞는 정의를 검색 후 현재 레벨 정의로 지정
	CurrentDefinition = (bIsTransitioning && PendingDefinition) ? PendingDefinition.Get() : FindLevelDefinitionForWorld(World);

	UE_LOG(LogCXLevelTransition, Log, TEXT("Current level definition: %s"), *GetNameSafe(CurrentDefinition));

	// 레벨 열기 = 첫 Scene 서브레벨까지 열린 상태
	// 이 시점은 플레이어 스폰 전이므로, 여기서 동기 로드하면 서브레벨의 PlayerStart를 스폰에 사용할 수 있음
	// 비동기 로드까지 기다리도록 Full Flush
	if (CurrentDefinition && CurrentDefinition->InitialScene)
	{
		if (UCXSceneSubsystem* SceneSubsystem = World->GetSubsystem<UCXSceneSubsystem>())
		{
			SceneSubsystem->LoadSceneLevelsImmediately(CurrentDefinition->InitialScene);
		}
	}
}

bool UCXLevelTransitionSubsystem::StartCameraFade(bool bInFadeOut, float InDuration, const FLinearColor& InColor, const FTimerDelegate& InOnComplete)
{
	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetGameInstance()->GetWorld(), 0);
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

	// 가림은 끝난 뒤에도 유지(bHoldWhenFinished), 공개는 끝나면 해제
	CameraManager->StartCameraFade(FromAlpha, ToAlpha, InDuration, InColor, /*bShouldFadeAudio*/ true, /*bHoldWhenFinished*/ bInFadeOut);

	// 월드가 바뀌어도 타이머가 유지되도록 GameInstance쪽 TimerManager 사용
	GetGameInstance()->GetTimerManager().SetTimer(FadeHandle, InOnComplete, InDuration, false);
	return true;
}

void UCXLevelTransitionSubsystem::OpenPendingLevel()
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (nullptr == World || nullptr == PendingDefinition)
	{
		UE_LOG(LogCXLevelTransition, Error, TEXT("OpenPendingLevel 실패: 월드 또는 목표 레벨 정의가 없습니다."));
		ResetTransition();
		return;
	}

	UGameplayStatics::OpenLevelBySoftObjectPtr(World, PendingDefinition->Level);
}

void UCXLevelTransitionSubsystem::FinishTransition()
{
	const UCXLevelDefinition* FinishedDefinition = PendingDefinition;
	ResetTransition();

	UE_LOG(LogCXLevelTransition, Log, TEXT("Level transition finished: %s"), *GetNameSafe(FinishedDefinition));

	OnTransitionFinished.Broadcast(FinishedDefinition);
}

void UCXLevelTransitionSubsystem::HandleTravelFailure(UWorld* InWorld, ETravelFailure::Type InFailureType, const FString& InErrorString)
{
	if (!bIsTransitioning)
	{
		return;
	}

	UE_LOG(LogCXLevelTransition, Error, TEXT("레벨 이동 실패: %s (%s) %s"),
		*GetNameSafe(PendingDefinition), ETravelFailure::ToString(InFailureType), *InErrorString);

	bScreenCovered = false;
	GetGameInstance()->GetTimerManager().ClearTimer(FadeHandle);
	ResetTransition();

	// 현재 월드에서 가림 해제
	if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(InWorld, 0))
	{
		CameraManager->StopCameraFade();
	}
}

void UCXLevelTransitionSubsystem::ResetTransition()
{
	bIsTransitioning = false;
	bIsRevealing = false;
	PendingDefinition = nullptr;
	PendingEntryTag = NAME_None;
}

#if !UE_BUILD_SHIPPING
void UCXLevelTransitionSubsystem::ValidateSettings() const
{
	const UCXLevelTransitionSettings* Settings = GetDefault<UCXLevelTransitionSettings>();
	if (nullptr == LoadLevelDefinition(Settings->TitleLevel))
	{
		UE_LOG(LogCXLevelTransition, Warning, TEXT("[TitleLevel] 레벨 정의를 찾을 수 없습니다. (Project Settings > Game > CXVI Level Transition)"));
	}

	if (nullptr == LoadLevelDefinition(Settings->NewGameLevel))
	{
		UE_LOG(LogCXLevelTransition, Warning, TEXT("[NewGameLevel] 레벨 정의를 찾을 수 없습니다. (Project Settings > Game > CXVI Level Transition)"));
	}

	if (Settings->TitleLevel.IsValid() && Settings->TitleLevel == Settings->NewGameLevel)
	{
		UE_LOG(LogCXLevelTransition, Warning, TEXT("TitleLevel과 NewGameLevel이 같은 레벨 정의입니다."));
	}
}
#endif // !UE_BUILD_SHIPPING
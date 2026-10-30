// Fill out your copyright notice in the Description page of Project Settings.


#include "CXGameScene.h"

#include "CXSceneDefinition.h"
#include "CXSceneSubsystem.h"
#include "CXSceneSuspendable.h"
#include "UI/CXActivatableWidget.h"
#include "UI/CXUIManagerSubsystem.h"
#include "Tags/CXGameplayTags.h"

#include <EnhancedInputSubsystems.h>
#include <InputMappingContext.h>
#include <Engine/LevelStreaming.h>
#include <Engine/LocalPlayer.h>
#include <EngineUtils.h>
#include <GameFramework/PlayerController.h>
#include <Kismet/GameplayStatics.h>
#include <GameFramework/GameModeBase.h>

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
	// OnEnter 처리가 컨트롤 Pawn이 준비된 상태에서 실행되도록 가장 먼저 처리
	AcquireScenePawn();
	PossessScenePawn();

	ApplyInputMappings();
	SetupSceneInput();
	ShowSceneHUD();

	// BP에서 정의한 처리 호출
	K2_OnEnter(InContext);
}

void UCXGameScene::OnExit()
{
	// 자식 BP에서 정의한 처리 먼저 호출
	K2_OnExit();

	HideSceneHUD();
	ClearSceneInput();
	ReleaseScenePawn();
	DestroySpawnedActors();
	RemoveInputMappings();
}

void UCXGameScene::OnSuspend()
{
	K2_OnSuspend();

	HideSceneHUD();
	ClearSceneInput();

	ReleasePossessionBeforeHide();
	NotifySuspendables(true);
	RemoveInputMappings();
}

void UCXGameScene::OnResume(const FCXSceneContext& InContext)
{
	// 숨김/정지가 풀린 뒤에 빙의
	NotifySuspendables(false);
	PossessScenePawn();

	ApplyInputMappings();
	SetupSceneInput();
	ShowSceneHUD();

	K2_OnResume(InContext);
}

void UCXGameScene::ShowSceneHUD()
{
	if (nullptr == Definition || nullptr == Definition->HUDClass)
	{
		return;
	}

	UCXUIManagerSubsystem* UIManager = UCXUIManagerSubsystem::TryGet(this);
	if (nullptr == UIManager)
	{
		UE_LOG(LogCXScene, Warning, TEXT("[%s] UIManager가 없어 HUD를 띄우지 못했습니다."), *GetName());
		return;
	}

	// Lambda는 Layer Stack에 추가될 때 전달 될 초기화 로직
	SceneHUD = UIManager->PushWidgetToLayer<UCXActivatableWidget>(CXVIGameplayTags::UI_Layer_Game, Definition->HUDClass,
		[this](UCXActivatableWidget& InHUD)
		{
			InitializeSceneHUD(InHUD);
		});

	if (false == SceneHUD.IsValid())
	{
		UE_LOG(LogCXScene, Warning, TEXT("[%s] HUD(%s) Push 실패 (PrimaryLayout 확인)"), *GetName(), *GetNameSafe(Definition->HUDClass));
	}
}

void UCXGameScene::HideSceneHUD()
{
	if (UCXActivatableWidget* HUD = SceneHUD.Get())
	{
		HUD->DeactivateWidget();
	}

	SceneHUD.Reset();
}

APlayerController* UCXGameScene::GetPlayerController() const
{
	return UGameplayStatics::GetPlayerController(this, 0);
}

UEnhancedInputComponent* UCXGameScene::GetPlayerInputComponent() const
{
	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	return PC ? Cast<UEnhancedInputComponent>(PC->InputComponent) : nullptr;
}

UEnhancedInputLocalPlayerSubsystem* UCXGameScene::GetInputSubsystem() const
{
	const UWorld* World = GetWorld();
	if (nullptr == World)
	{
		return nullptr;
	}

	const UGameInstance* GameInstance = World->GetGameInstance();
	if (nullptr == GameInstance)
	{
		return nullptr;
	}

	const ULocalPlayer* LocalPlayer = GameInstance->GetFirstGamePlayer();
	if(nullptr == LocalPlayer)
	{
		return nullptr;
	}

	return LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
}

void UCXGameScene::ApplyInputMappings() const
{
	if (nullptr == Definition)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetInputSubsystem();
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

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetInputSubsystem();
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
	// 대상 = 이 Scene 레벨의 액터 + 이 Scene의 조종 Pawn
	// 퍼시스턴트 레벨 전체를 대상으로 하면 다른 Scene의 Suspend/Resume에도 알림이 가므로 제외
	// -> Suspend는 Hide 전에, Resume은 Show 후에 호출되도록 Subsystem의 단계 순서로 보장
	TArray<AActor*> Targets;
	for (const ULevel* Level : GetSceneLevels())
	{
		for (AActor* Actor : Level->Actors)
		{
			if (IsValid(Actor))
			{
				Targets.Emplace(Actor);
			}
		}
	}

	// 퍼시스턴트에 생성한 Pawn (서브레벨에 배치된 Pawn은 위에서 이미 포함)
	if (IsValid(ScenePawn))
	{
		Targets.AddUnique(ScenePawn);
	}

	for (AActor* Actor : Targets)
	{
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

void UCXGameScene::AcquireScenePawn()
{
	if (nullptr == Definition || nullptr == Definition->PawnClass)
	{
		return;
	}

	if (APawn* FoundPawn = FindPawnInSceneLevels())
	{
		ScenePawn = FoundPawn;
		bSpawnedScenePawn = false;
		return;
	}

	ScenePawn = SpawnScenePawn();
	bSpawnedScenePawn = (nullptr != ScenePawn);

	if (nullptr == ScenePawn)
	{
		UE_LOG(LogCXScene, Error, TEXT("[%s] 조종 Pawn(%s)을 찾거나 생성하지 못했습니다."), *GetName(), *GetNameSafe(Definition->PawnClass));
	}
}

APawn* UCXGameScene::FindPawnInSceneLevels() const
{
	TArray<APawn*> Pawns;
	FindActorsInSceneLevels(Pawns);

	Pawns.RemoveAll([this](const APawn* InPawn)
		{
			return false == InPawn->IsA(Definition->PawnClass);
		});

	if (Pawns.Num() > 1)
	{
		UE_LOG(LogCXScene, Warning, TEXT("[%s] Scene 레벨에 %s가 %d개 있습니다. 첫 번째를 사용합니다."), *GetName(), *GetNameSafe(Definition->PawnClass), Pawns.Num());
	}

	return Pawns.IsEmpty() ? nullptr : Pawns[0];
}

APawn* UCXGameScene::SpawnScenePawn() const
{
	UWorld* World = GetWorld();
	if (nullptr == World )
	{
		return nullptr;
	}

	APlayerController* PC = GetPlayerController();
	if (nullptr == PC)
	{
		return nullptr;
	}
	
	AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
	if (nullptr == GameMode)
	{
		return nullptr;
	}

	// 로그인 때 진입 태그로 고른 PlayerStart(StartSpot)를 그대로 사용
	const AActor* StartSpot = GameMode->FindPlayerStart(PC);
	if (nullptr == StartSpot)
	{
		UE_LOG(LogCXScene, Warning, TEXT("[%s] 조종 Pawn을 생성할 PlayerStart가 없습니다."), *GetName());
		return nullptr;
	}

	// 퍼시스턴트 레벨에 Pawn 생성
	// Scene 레벨이 Hidden 되어도 월드에서 빠지지 않음 (숨김 / 정지는 ICXSceneSuspendable에서 따로 처리가 필요)
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	return World->SpawnActor<APawn>(Definition->PawnClass, StartSpot->GetActorLocation(), StartSpot->GetActorRotation(), Params);
}

void UCXGameScene::PossessScenePawn()
{
	APlayerController* PC = GetPlayerController();
	if (nullptr == PC || false == IsValid(ScenePawn) || PC->GetPawn() == ScenePawn)
	{
		return;
	}

	// Possess가 이전 Pawn 빙의 해제, 컨트롤 회전, 뷰 타깃 전환(bAutoManageActiveCameraTarget)까지 처리
	PC->Possess(ScenePawn);
}

void UCXGameScene::ReleasePossessionBeforeHide()
{
	APlayerController* PC = GetPlayerController();
	if (nullptr == PC || false == IsValid(ScenePawn) || PC->GetPawn() != ScenePawn)
	{
		return;
	}

	// 서브레벨 Pawn은 레벨이 Hidden 되면 월드에서 빠지므로 그 전에 PlayerController 해제
	// 퍼시스턴트 Pawn은 유지 -> 위 Scene이 자기 Pawn을 빙의할 때 자동 해제되고, 메뉴처럼 Pawn이 없는 Scene에서는 카메라 유지
	if (false == ScenePawn->GetLevel()->IsPersistentLevel())
	{
		PC->UnPossess();
	}
}

void UCXGameScene::ReleaseScenePawn()
{
	APlayerController* PC = GetPlayerController();
	if (PC && IsValid(ScenePawn) && PC->GetPawn() == ScenePawn)
	{
		PC->UnPossess();
	}

	if (bSpawnedScenePawn && IsValid(ScenePawn))
	{
		ScenePawn->Destroy();
	}

	ScenePawn = nullptr;
	bSpawnedScenePawn = false;
}

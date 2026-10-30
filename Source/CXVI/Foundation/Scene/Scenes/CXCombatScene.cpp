// Fill out your copyright notice in the Description page of Project Settings.


#include "CXCombatScene.h"

#include "Foundation/Scene/CXSceneSubsystem.h"
#include "Foundation/Combat/CXCombatManagerSubsystem.h"
#include "Foundation/Combat/CombatMessage/CXCombatMessageData.h"
#include "Actor/Combat/CXCombatArena.h"
#include "Character/Combat/CXCommandCharacter.h"
#include "Tags/CXGameplayTags.h"
#include "UI/Combat/CXCombatHUDWidget.h"
#include "UI/ViewModel/CXCombatViewModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatScene)

void UCXCombatScene::OnEnter(const FCXSceneContext& InContext)
{
	// 베이스의 HUD Push에서 주입할 수 있도록 먼저 생성 (전투원은 StartBattle에서 채움)
	CombatViewModel = NewObject<UCXCombatViewModel>(this);

	// 베이스에서 커맨드 Pawn 빙의 + IMC 등록 (전투 준비보다 먼저)
	Super::OnEnter(InContext);

	CombatEndListener = UCXGameplayMessageSubsystem::Get(this).RegisterListener(CXVIGameplayTags::Message_Combat_End, this, &ThisClass::HandleCombatEnded);

	if (!StartBattle(InContext))
	{
		// 전투 레벨에 갇히지 않도록 바로 복귀 요청 (진입 전환 중이므로 전환이 끝난 뒤 Pop)
		UE_LOG(LogCXScene, Error, TEXT("CombatScene : 전투를 시작하지 못해 필드로 돌아갑니다."));
		EndCombat(FCXCombatResult());
	}
}

void UCXCombatScene::OnExit()
{
	CombatEndListener.Unregister();
	UnbindSceneChanged();

	// 화면이 가려진 상태에서 정리. 턴 정지(바인딩 해제)가 전투원 Destroy보다 먼저
	if (UCXCombatManagerSubsystem* CombatManager = GetCombatManager())
	{
		CombatManager->EndBattle();
	}

	// 전투원들이 파괴 되기전에 ViewModel에 바인딩 된 ASC 구독 해제
	if (CombatViewModel)
	{
		CombatViewModel->Reset();
		CombatViewModel = nullptr;
	}

	if (ACXCombatArena* CombatArena = Arena.Get())
	{
		CombatArena->CleanupCombatants();
	}
	Arena.Reset();

	// 베이스에서 입력 해제 + 빙의 해제 + IMC 제거
	Super::OnExit();
}

void UCXCombatScene::InitializeSceneHUD(UCXActivatableWidget& InHUD)
{
	Super::InitializeSceneHUD(InHUD);

	if (UCXCombatHUDWidget* CombatHUD = Cast<UCXCombatHUDWidget>(&InHUD))
	{
		CombatHUD->SetCombatViewModel(CombatViewModel);
	}
	else
	{
		UE_LOG(LogCXScene, Warning, TEXT("CombatScene : HUDClass(%s)가 UCXCombatHUDWidget이 아닙니다."), *GetNameSafe(InHUD.GetClass()));
	}
}

bool UCXCombatScene::StartBattle(const FCXSceneContext& InContext)
{
	const FCXCombatEnterParams* Params = InContext.GetPayload<FCXCombatEnterParams>();
	if (nullptr == Params)
	{
		UE_LOG(LogCXScene, Error, TEXT("CombatScene : FCXCombatEnterParams가 없습니다."));
		return false;
	}

	UCXCombatManagerSubsystem* CombatManager = GetCombatManager();
	if (nullptr == CombatManager)
	{
		return false;
	}

	TArray<ACXCombatArena*> Arenas;
	FindActorsInSceneLevels(Arenas);
	if (Arenas.Num() != 1)
	{
		UE_LOG(LogCXScene, Error, TEXT("CombatScene : 전투 레벨의 아레나가 %d개입니다. (1개 필요)"), Arenas.Num());
		return false;
	}

	ACXCommandCharacter* CommandPawn = Cast<ACXCommandCharacter>(GetScenePawn());
	if (nullptr == CommandPawn)
	{
		UE_LOG(LogCXScene, Error, TEXT("CombatScene : 커맨드 Pawn이 없습니다. (Definition의 PawnClass 확인)"));
		return false;
	}

	ACXCombatArena* CombatArena = Arenas[0];
	if (!CombatArena->SpawnCombatants(*Params))
	{
		return false;
	}
	Arena = CombatArena;

	// 전투원들의 ViewModel을 생성
	// TODO : 보스몬스터가 추가 되면 2번째 인자에 추가
	CombatViewModel->SetCombatants(CombatArena->GetPlayers(), /*Boss*/nullptr);

	FCXBattleSetup Setup;
	Setup.Arena = CombatArena;
	Setup.CommandPawn = CommandPawn;
	Setup.Players = CombatArena->GetPlayers();
	Setup.Enemies = CombatArena->GetEnemies();

	return CombatManager->BeginBattle(Setup);
}

void UCXCombatScene::EndCombat(const FCXCombatResult& InResult)
{
	UCXSceneSubsystem* Subsystem = GetSceneSubsystem();
	if (nullptr == Subsystem)
	{
		return;
	}

	// 전환 중에는 Pop이 거부되므로 전환이 끝난 뒤 처리
	if (Subsystem->IsTransitioning())
	{
		PendingResult = InResult;
		if (!SceneChangedHandle.IsValid())
		{
			SceneChangedHandle = Subsystem->OnSceneChanged.AddUObject(this, &ThisClass::HandleSceneChanged);
		}
		return;
	}

	Subsystem->PopScene(FCXSceneContext::Make(InResult));
}

void UCXCombatScene::HandleCombatEnded(const FGameplayTag& InChannel, const FCXCombatEndMessage& InMessage)
{
	FCXCombatResult Result;
	Result.bVictory = InMessage.bVictory;

	EndCombat(Result);
}

void UCXCombatScene::HandleSceneChanged(UCXGameScene* InNewScene, UCXGameScene* InOldScene)
{
	UnbindSceneChanged();

	// FinishTransition에서 전환 상태를 푼 뒤 방송하므로 여기서는 Pop 가능
	EndCombat(PendingResult);
}

void UCXCombatScene::UnbindSceneChanged()
{
	if (SceneChangedHandle.IsValid())
	{
		if (UCXSceneSubsystem* Subsystem = GetSceneSubsystem())
		{
			Subsystem->OnSceneChanged.Remove(SceneChangedHandle);
		}

		SceneChangedHandle.Reset();
	}
}

UCXCombatManagerSubsystem* UCXCombatScene::GetCombatManager() const
{
	return UCXCombatManagerSubsystem::TryGet(this);
}

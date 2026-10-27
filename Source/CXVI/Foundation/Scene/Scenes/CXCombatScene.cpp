// Fill out your copyright notice in the Description page of Project Settings.


#include "CXCombatScene.h"

#include "Foundation/Scene/CXSceneSubsystem.h"
#include "Foundation/Combat/CXCombatManagerSubsystem.h"
#include "Foundation/Combat/CombatMessage/CXCombatMessageData.h"
#include "Tags/CXGameplayTags.h"

#include <Kismet/GameplayStatics.h>
#include <Camera/CameraActor.h>


#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatScene)

void UCXCombatScene::OnEnter(const FCXSceneContext& InContext)
{
	Super::OnEnter(InContext);

	//const FCXCombatEnterParams* Params = InContext.GetPayload<FCXCombatEnterParams>();
	//if (nullptr == Params)
	//{
	//	UE_LOG(LogCXScene, Error, TEXT("CombatScene: FCXCombatEnterParams가 없습니다."));
	//	return;
	//}

	//SetupCombatCamera();
	//SpawnCombatants(*Params);

	// TODO : 전투 전환
	CombatEndListener = UCXGameplayMessageSubsystem::Get(this).RegisterListener(CXVIGameplayTags::Message_Combat_End, this, &ThisClass::HandleCombatEnded);

	const FCXCombatEnterParams* Params = InContext.GetPayload<FCXCombatEnterParams>();
	UCXCombatManagerSubsystem* CombatManager = GetCombatManager();

	FCXArenaSetupData SetupData;
	if (Params)
	{
		SetupData.PlayerClasses = Params->PlayerClasses;
		SetupData.EnemyClasses = Params->EnemyClasses;
	}

	const bool bStarted = Params && CombatManager &&
		CombatManager->StartSceneCombat(SetupData, GetSceneLevels(), UGameplayStatics::GetPlayerController(this, 0));

	if (!bStarted)
	{
		// 전투 레벨에 갇히지 않도록 바로 복귀 요청 (진입 전환 중이므로 전환이 끝난 뒤 Pop)
		UE_LOG(LogCXScene, Error, TEXT("CombatScene: 전투를 시작하지 못해 필드로 돌아갑니다."));
		EndCombat(FCXCombatResult());
	}
}

void UCXCombatScene::OnExit()
{
	//if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	//{
	//	PC->SetViewTarget(PC->GetPawn());
	//}

	CombatEndListener.Unregister();
	UnbindSceneChanged();

	// 화면이 가려진 상태에서 전투원 정리
	if (UCXCombatManagerSubsystem* CombatManager = GetCombatManager())
	{
		CombatManager->CleanupSceneCombat();
	}

	// 베이스에서 입력 해제 + 전투원 Destroy + IMC 제거
	Super::OnExit();
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

void UCXCombatScene::SetupCombatCamera()
{
	// Combat Scene에 배치한 카메라를 찾음
	TArray<ACameraActor*> Cameras;
	FindActorsInSceneLevels(Cameras);

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (nullptr == PC || Cameras.IsEmpty())
	{
		UE_LOG(LogCXScene, Warning, TEXT("CombatScene: 전투 레벨에 CameraActor가 없습니다."));
		return;
	}

	PC->SetViewTarget(Cameras[0]);
}

void UCXCombatScene::SpawnCombatants(const FCXCombatEnterParams& InParams)
{
	// TODO : 전투 캐릭터들 생성
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
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UCXCombatManagerSubsystem>() : nullptr;
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "CXCombatScene.h"

#include "Foundation/Scene/CXSceneSubsystem.h"
#include <Kismet/GameplayStatics.h>
#include <Camera/CameraActor.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatScene)

void UCXCombatScene::OnEnter(const FCXSceneContext& InContext)
{
	Super::OnEnter(InContext);

	const FCXCombatEnterParams* Params = InContext.GetPayload<FCXCombatEnterParams>();
	if (nullptr == Params)
	{
		UE_LOG(LogCXScene, Error, TEXT("CombatScene: FCXCombatEnterParams가 없습니다."));
		return;
	}

	SetupCombatCamera();
	SpawnCombatants(*Params);

	// TODO : 전투 초기화 및 시작
}

void UCXCombatScene::OnExit()
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetViewTarget(PC->GetPawn());
	}

	// 베이스에서 입력 해제 + 전투원 Destroy + IMC 제거
	Super::OnExit();
}

void UCXCombatScene::EndCombat(const FCXCombatResult& InResult)
{
	if (UCXSceneSubsystem* Subsystem = GetSceneSubsystem())
	{
		Subsystem->PopScene(FCXSceneContext::Make(InResult));
	}
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

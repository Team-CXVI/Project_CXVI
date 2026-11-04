// Fill out your copyright notice in the Description page of Project Settings.


#include "CXFieldScene.h"
#include "CXVI/Foundation/Scene/Scenes/CXCombatScene.h"
#include "CXVI/Character/Field/CXFieldEnemy.h"
#include "CXVI/Foundation/Field/FieldEnemySpawning/CXFieldEnemySubsystem.h"
#include "CXVI/Foundation/Field/FieldEnemySpawning/CXFieldEnemySpawner.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldScene)

void UCXFieldScene::OnResume(const FCXSceneContext& InContext)
{
	Super::OnResume(InContext);

	const FCXCombatResult* Result = InContext.GetPayload<FCXCombatResult>();

	if (nullptr == Result)
	{
		return;
	}

	UWorld* World = GetWorld();
	UCXFieldEnemySubsystem* EnemySubsystem = IsValid(World) ? World->GetSubsystem<UCXFieldEnemySubsystem>() : nullptr;
	if (!IsValid(EnemySubsystem))
	{
		return;
	}

	const FName SpawnId = EnemySubsystem->GetSpawnedId();
	EnemySubsystem->SetSpawnedId(NAME_None);

	if (!Result->bVictory || SpawnId.IsNone())
	{
		return;
	}

	ACXFieldEnemySpawner* Spawner = EnemySubsystem->FindSpawner(SpawnId);
	ACXFieldEnemy* Enemy = IsValid(Spawner) ? Spawner->GetSpawnedEnemy() : nullptr;
	if (IsValid(Enemy))
	{
		Enemy->Defeat();
	}
}

void UCXFieldScene::OnSuspend()
{
	Super::OnSuspend();
}

void UCXFieldScene::OnExit()
{
	Super::OnExit();
}

void UCXFieldScene::SetupSceneInput()
{
	Super::SetupSceneInput();
}
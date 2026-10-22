// Fill out your copyright notice in the Description page of Project Settings.


#include "SubSystem/Field/Spawner/CXFieldEnemySpawner.h"
#include "SubSystem/Field/CXFieldMonsterSubsystem.h"

#include "Components/ArrowComponent.h"
#include "Engine/World.h"


// Sets default values
ACXFieldEnemySpawner::ACXFieldEnemySpawner()
{
	SpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnPoint"));
	SetRootComponent(SpawnPoint);
}

// Called when the game starts or when spawned
void ACXFieldEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	UCXFieldMonsterSubsystem* MonsterSubsystem = GetWorld()->GetSubsystem<UCXFieldMonsterSubsystem>();

	if (!MonsterSubsystem)
	{
		return;
	}

	const bool bRegistered = MonsterSubsystem->RegisterSpawnPoint(SpawnId, RestGroupId, bRespawnOnRest);

	if (!bRegistered)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 스폰 지점 등록에 실패했습니다."),*GetName());

		return;
	}

}


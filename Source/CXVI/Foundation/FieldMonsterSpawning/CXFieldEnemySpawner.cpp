// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Actor/Field/CXFieldEnemySpawner.h"
#include "CXFieldMonsterSubsystem.h"
#include "CXVI/Character/Field/CXFieldEnemy.h"


#include "Components/ArrowComponent.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldEnemySpawner)

// Sets default values
ACXFieldEnemySpawner::ACXFieldEnemySpawner()
{
	SpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnPoint"));
	SetRootComponent(SpawnPoint);
}

ACXFieldEnemy* ACXFieldEnemySpawner::SpawnEnemy()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	UCXFieldMonsterSubsystem* MonsterSubsystem = World->GetSubsystem<UCXFieldMonsterSubsystem>();
	if (!IsValid(MonsterSubsystem))
	{
		return nullptr;
	}

	// 등록 여부와 처치 여부를 확인.
	bool bDefeated = false;

	const bool bFound = MonsterSubsystem->TryGetMonsterDefeated(SpawnId, bDefeated);

	if (!bFound || bDefeated)
	{
		return nullptr;
	}

	// 같은 스포너에서 중복 생성하지 않음.
	if (nullptr != SpawnedEnemy)
	{
		return SpawnedEnemy.Get();
	}

	if (!IsValid(EnemyClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: EnemyClass가 지정되지 않았습니다."), *GetName());


		return nullptr;
	}

	FActorSpawnParameters SpawnParams;

	SpawnParams.Owner = this;

	// 몬스터를 스포너와 같은 레벨에 생성.
	SpawnParams.OverrideLevel = GetLevel();

	// 지점 위치에 충돌이 있으면 생성하지 않음.
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;

	SpawnedEnemy = World->SpawnActor<ACXFieldEnemy>(EnemyClass, SpawnPoint->GetComponentTransform(), SpawnParams);

	if (nullptr == SpawnedEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 몬스터 생성에 실패했습니다. 생성 위치의 충돌을 확인하세요."), *GetName());

	}

	return SpawnedEnemy.Get();
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
		UE_LOG(LogTemp, Warning, TEXT("%s: 스폰 지점 등록에 실패했습니다."), *GetName());

		return;
	}

	// 등록된 상태를 확인하고 몬스터 생성.
	SpawnEnemy();

}

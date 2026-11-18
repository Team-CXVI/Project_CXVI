// Fill out your copyright notice in the Description page of Project Settings.


#include "CXFieldItemSpawner.h"
#include "CXVI/Actor/Field/Interactions/CXFieldItemPickup.h"
#include "CXVI/Foundation/UserData/CXUserDataSubsystem.h"

#include "Engine/World.h"
#include "Components/ArrowComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldItemSpawner)

// Sets default values
ACXFieldItemSpawner::ACXFieldItemSpawner()
{
	SpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnPoint"));
	SetRootComponent(SpawnPoint);
}

ACXFieldItemPickup* ACXFieldItemSpawner::SpawnPickup()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	if (SpawnId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: SpawnId가 지정되지 않았습니다."), *GetName());

		return nullptr;
	}

	// 이미 획득한 아이템은 생성하지 않음.
	UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this);
	if (!IsValid(UserData) || UserData->IsItemCollected(SpawnId))
	{
		return nullptr;
	}

	// 같은 스포너에서 중복 생성하지 않음.
	if (IsValid(SpawnedPickup))
	{
		return SpawnedPickup.Get();
	}

	if (!IsValid(PickupClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: PickupClass가 지정되지 않았습니다."), *GetName());

		return nullptr;
	}

	FActorSpawnParameters SpawnParams;

	SpawnParams.Owner = this;

	// 아이템을 스포너와 같은 레벨에 생성.
	SpawnParams.OverrideLevel = GetLevel();

	// 배치한 위치 그대로 생성.
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 생성 완료를 미루고 먼저 설정을 전달하기 위함.
	SpawnParams.bDeferConstruction = true;

	const FTransform SpawnTransform = SpawnPoint->GetComponentTransform();

	SpawnedPickup = World->SpawnActor<ACXFieldItemPickup>(PickupClass, SpawnTransform, SpawnParams);

	if (!IsValid(SpawnedPickup))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 아이템 생성에 실패했습니다."), *GetName());

		return nullptr;
	}

	// 생성 완료 전에 설정 전달.
	SpawnedPickup->InitializePickup(SpawnId, Rewards);

	SpawnedPickup->FinishSpawning(SpawnTransform);

	if (!IsValid(SpawnedPickup))
	{
		return nullptr;
	}

	return SpawnedPickup.Get();
}

// Called when the game starts or when spawned
void ACXFieldItemSpawner::BeginPlay()
{
	Super::BeginPlay();

	// 획득 기록을 확인하고 아이템 생성.
	SpawnPickup();
}

void ACXFieldItemSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 스포너가 종료되면 담당 아이템도 정리.
	if (IsValid(SpawnedPickup))
	{
		SpawnedPickup->Destroy();
	}

	SpawnedPickup = nullptr;

	Super::EndPlay(EndPlayReason);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "CXSaveFlag.h"
#include "CXSaveFlagTriggerBox.h"
#include "CXVI/Foundation/FieldEnemySpawning/CXFieldEnemySubsystem.h"
#include "CXVI/Character/Field/CXFieldPlayerCharacter.h"

#include "Engine/World.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ChildActorComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(CXSaveFlag)

// Sets default values
ACXSaveFlag::ACXSaveFlag()
{
    USceneComponent* SceneRoot =
        CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

    SetRootComponent(SceneRoot);

    ExpeditionFlag =
        CreateDefaultSubobject<UStaticMeshComponent>(
            TEXT("ExpeditionFlag"));
    ExpeditionFlag->SetupAttachment(SceneRoot);

    SaveFlagTrigger =
        CreateDefaultSubobject<UChildActorComponent>(
            TEXT("SaveFlagTrigger"));
    SaveFlagTrigger->SetupAttachment(SceneRoot);
    SaveFlagTrigger->SetChildActorClass(
        ACXSaveFlagTriggerBox::StaticClass());
    SaveFlagTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
}

bool ACXSaveFlag::CanRest(const ACXFieldPlayerCharacter* Player) const
{
    if (!IsValid(Player) || !IsValid(SaveFlagTrigger))
    {
        return false;
    }

    const AActor* Trigger = SaveFlagTrigger->GetChildActor();
    if (!IsValid(Trigger))
    {
        return false;
    }

    return Trigger->IsOverlappingActor(Player);
}

void ACXSaveFlag::CompleteRest()
{
    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        return;
    }

    UCXFieldEnemySubsystem* EnemySubsystem = World->GetSubsystem<UCXFieldEnemySubsystem>();
    if (!IsValid(EnemySubsystem))
    {
        return;
    }
    const int32 RespawnedCount = EnemySubsystem->RespawnDefeatedEnemies(NAME_None);

    UE_LOG(LogTemp, Log, TEXT("깃발 휴식: 필드 적 %d 마리 복원"), RespawnedCount);
}

// Called when the game starts or when spawned
void ACXSaveFlag::BeginPlay()
{
	Super::BeginPlay();
	
}




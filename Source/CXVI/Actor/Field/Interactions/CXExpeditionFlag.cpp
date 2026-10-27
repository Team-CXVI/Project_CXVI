// Fill out your copyright notice in the Description page of Project Settings.


#include "CXExpeditionFlag.h"
#include "CXExpeditionFlagTriggerBox.h"
#include "CXVI/Foundation/FieldEnemySpawning/CXFieldEnemySubsystem.h"
#include "CXVI/Character/Field/CXFieldPlayerCharacter.h"

#include "Engine/World.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ChildActorComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(CXExpeditionFlag)

// Sets default values
ACXExpeditionFlag::ACXExpeditionFlag()
{
    USceneComponent* SceneRoot =
        CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

    SetRootComponent(SceneRoot);

    ExpeditionFlag =
        CreateDefaultSubobject<UStaticMeshComponent>(
            TEXT("ExpeditionFlag"));
    ExpeditionFlag->SetupAttachment(SceneRoot);

    // Keep the serialized subobject name so existing Blueprint overrides remain valid.
    ExpeditionFlagTrigger =
        CreateDefaultSubobject<UChildActorComponent>(
            TEXT("SaveFlagTrigger"));
    ExpeditionFlagTrigger->SetupAttachment(SceneRoot);
    ExpeditionFlagTrigger->SetChildActorClass(
        ACXExpeditionFlagTriggerBox::StaticClass());
    ExpeditionFlagTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
}

bool ACXExpeditionFlag::CanFlag(const ACXFieldPlayerCharacter* Player) const
{
    if (!IsValid(Player) || !IsValid(ExpeditionFlagTrigger))
    {
        return false;
    }

    const AActor* Trigger = ExpeditionFlagTrigger->GetChildActor();
    if (!IsValid(Trigger))
    {
        return false;
    }

    return Trigger->IsOverlappingActor(Player);
}

void ACXExpeditionFlag::CompleteFlag()
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
void ACXExpeditionFlag::BeginPlay()
{
	Super::BeginPlay();
	
}




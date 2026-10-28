// Fill out your copyright notice in the Description page of Project Settings.


#include "CXExpeditionFlag.h"
#include "CXVI/Foundation/Field/FieldEnemySpawning/CXFieldEnemySubsystem.h"
#include "CXVI/Character/Field/CXFieldPlayerCharacter.h"
#include "CXVI/Tags/CXFieldTags.h"

#include "Engine/World.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SphereComponent.h"


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
    //ExpeditionFlagTrigger =
    //    CreateDefaultSubobject<UChildActorComponent>(
    //        TEXT("SaveFlagTrigger"));
    //ExpeditionFlagTrigger->SetupAttachment(SceneRoot);
    //ExpeditionFlagTrigger->SetChildActorClass(
    //    ACXExpeditionFlagTriggerBox::StaticClass());
    //ExpeditionFlagTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));

    ExpeditionInteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ExpeditionInteractionSphere"));
    ExpeditionInteractionSphere->SetupAttachment(SceneRoot);

    ExpeditionInteractionSphere->SetRelativeLocation(FVector::ZeroVector);
    ExpeditionInteractionSphere->SetSphereRadius(100.0f);
    ExpeditionInteractionSphere->SetGenerateOverlapEvents(true);
}

bool ACXExpeditionFlag::CanFlag(const ACXFieldPlayerCharacter* Player) const
{
    if (!IsValid(Player) || !IsValid(ExpeditionInteractionSphere))
    {
        return false;
    }

    return true;
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
	
    ExpeditionInteractionSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleInteractionBeginOverlap);
    ExpeditionInteractionSphere->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::HandleInteractionEndOverlap);
}

void ACXExpeditionFlag::HandleInteractionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(OtherActor);

    if (!IsValid(Player))
    {
        return;
    }

    if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
    {
        // 그래플링 훅 처럼해당 타겟에 UI.

        ASC->AddLooseGameplayTag(CXVIFieldTag::State_CanUseExpeditionFlag);
    }

    ACXExpeditionFlag* Flag = this;
    if (!IsValid(Flag))
    {
        return;
    }

    Player->RegisterNearByFlag(Flag);
}

void ACXExpeditionFlag::HandleInteractionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(OtherActor);

    if (!IsValid(Player))
    {
        return;
    }

    if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
    {
        // 그래플링 훅 처럼해당 타겟에 UI.

        ASC->RemoveLooseGameplayTag(CXVIFieldTag::State_CanUseExpeditionFlag);
    }

    ACXExpeditionFlag* Flag = Cast<ACXExpeditionFlag>(GetParentActor());
    if (!IsValid(Flag))
    {
        return;
    }

    Player->UnregisterNearByFlag(Flag);
}




// Fill out your copyright notice in the Description page of Project Settings.


#include "CXSaveFlag.h"
#include "CXSaveFlagTriggerBox.h"

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

// Called when the game starts or when spawned
void ACXSaveFlag::BeginPlay()
{
	Super::BeginPlay();
	
}




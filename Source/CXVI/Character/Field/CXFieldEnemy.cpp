// Fill out your copyright notice in the Description page of Project Settings.


#include "CXFieldEnemy.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "Interactions/CXPreemptiveAttackTriggerBox.h"

ACXFieldEnemy::ACXFieldEnemy( )
{
	PreemptiveAttackTrigger = CreateDefaultSubobject<UChildActorComponent>(TEXT("PreemptiveAttackTrigger"));
	
	PreemptiveAttackTrigger->SetupAttachment(GetRootComponent());

	PreemptiveAttackTrigger->SetChildActorClass(ACXPreemptiveAttackTriggerBox::StaticClass());

	PreemptiveAttackTrigger->SetRelativeLocation(FVector::ZeroVector);

}

void ACXFieldEnemy::BeginPlay()
{
	Super::BeginPlay();

}



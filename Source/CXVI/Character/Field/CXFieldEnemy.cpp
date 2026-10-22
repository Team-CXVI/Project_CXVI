// Fill out your copyright notice in the Description page of Project Settings.


#include "CXFieldEnemy.h"
#include "CXVI/AI/Field/CXFieldAIController.h"


#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "Interactions/CXPreemptiveAttackTriggerBox.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldEnemy)

ACXFieldEnemy::ACXFieldEnemy()
{
	PreemptiveAttackTrigger = CreateDefaultSubobject<UChildActorComponent>(TEXT("PreemptiveAttackTrigger"));

	PreemptiveAttackTrigger->SetupAttachment(GetRootComponent());

	PreemptiveAttackTrigger->SetChildActorClass(ACXPreemptiveAttackTriggerBox::StaticClass());

	PreemptiveAttackTrigger->SetRelativeLocation(FVector::ZeroVector);


	// 빙의(Possess) 옵션 설정.
	// 월드 공간에 미리 배치된 경우 및 런타임에 생성된 경우 모두 aicontroller에 빙의.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	
}

void ACXFieldEnemy::BeginPlay()
{
	Super::BeginPlay();


}

float ACXFieldEnemy::GetAIPatrolRadius()
{
	return 500.0f;
}

float ACXFieldEnemy::GetAIDetectRange()
{
	return 400.0f;
}

float ACXFieldEnemy::GetAIAttackRange()
{
	return 0.0f;
}

float ACXFieldEnemy::GetAITurnSpeed()
{
	return 0.0f;
}



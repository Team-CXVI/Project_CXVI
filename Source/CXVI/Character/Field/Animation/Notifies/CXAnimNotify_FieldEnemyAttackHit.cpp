// Fill out your copyright notice in the Description page of Project Settings.


#include "CXAnimNotify_FieldEnemyAttackHit.h"
#include "Character/Field/CXFieldEnemy.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXAnimNotify_FieldEnemyAttackHit)

void UCXAnimNotify_FieldEnemyAttackHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	UWorld* World = MeshComp->GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	ACXFieldEnemy* Enemy = Cast<ACXFieldEnemy>(MeshComp->GetOwner());

	if (IsValid(Enemy))
	{
		Enemy->NotifyAttackHit();
	}
}

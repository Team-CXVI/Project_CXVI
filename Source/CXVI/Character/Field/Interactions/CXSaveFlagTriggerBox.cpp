// Fill out your copyright notice in the Description page of Project Settings.


#include "CXSaveFlagTriggerBox.h"
#include "../CXFieldPlayerCharacter.h"
#include "../../../Tags/CXFieldTags.h"

#include "Components/ShapeComponent.h"
#include "AbilitySystemComponent.h"
ACXSaveFlagTriggerBox::ACXSaveFlagTriggerBox(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	OnActorBeginOverlap.AddDynamic(this, &ThisClass::HandleActorBeginOverlap);
	OnActorEndOverlap.AddDynamic(this, &ThisClass::HandleActorEndOverlap);

	// 트리거 충돌 설정.
	if (UShapeComponent* Collision = GetCollisionComponent())
	{
		Collision->SetCollisionProfileName(TEXT("Trigger"));
		Collision->SetGenerateOverlapEvents(true);
	}
}

void ACXSaveFlagTriggerBox::HandleActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(OtherActor);

	if (!IsValid(Player))
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
	{
		// 그래플링 훅 처럼해당 타겟에 UI.

		ASC->RemoveLooseGameplayTag(CXCanSaveTag);
	}

}

void ACXSaveFlagTriggerBox::HandleActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(OtherActor);

	if (!IsValid(Player))
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
	{
		// 그래플링 훅 처럼해당 타겟에 UI.

		ASC->AddLooseGameplayTag(CXCanSaveTag);
	}

	
}

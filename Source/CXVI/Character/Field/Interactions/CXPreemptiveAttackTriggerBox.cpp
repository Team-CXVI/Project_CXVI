// Fill out your copyright notice in the Description page of Project Settings.


#include "CXPreemptiveAttackTriggerBox.h"

#include "../CXFieldPlayerCharacter.h"
#include "../CXFieldEnemy.h"

#include "AbilitySystemComponent.h"
#include "../../../Tags/CXFieldTags.h"
#include "Components/ShapeComponent.h"

ACXPreemptiveAttackTriggerBox::ACXPreemptiveAttackTriggerBox(const FObjectInitializer& ObjectInitializer)
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

void ACXPreemptiveAttackTriggerBox::HandleActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(OtherActor);
	if (nullptr == Player)
	{
		return;
	}


	if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
	{

		ASC->RemoveLooseGameplayTag(CXAssassinationTag);
	}

}

void ACXPreemptiveAttackTriggerBox::HandleActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(OtherActor);
	if (nullptr == Player)
	{
		return;
	}

	// #Todo: 에너미 캐릭터를 기준으로 등뒤인지 아니면 아닌지 조건을 맞춰야됨.
	EnemyCharacter = Cast<ACXFieldEnemy>(GetParentActor());
	if (nullptr == EnemyCharacter)
	{
		return;
	}


	FVector EnemyLocation = EnemyCharacter->GetActorLocation();
	FVector PlayerLocation = Player->GetActorLocation();

	

	FVector ToPlayer = (PlayerLocation - EnemyLocation).GetSafeNormal2D();

	float DirectionDot = FVector::DotProduct(EnemyCharacter->GetActorForwardVector().GetSafeNormal2D(), ToPlayer);
	


	if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
	{
		// 적 뒤쪽 약 120도.
		if (DirectionDot < -0.5f)
		{
			ASC->AddLooseGameplayTag(CXAssassinationTag);
		}
		//else
		//{
		//	if (/*적의 공격보다 빨랐으면*/)
		//	{
		//		ASC->AddLooseGameplayTag(/*선제 공격 가능 태그*/)
		//	}
		//}
		
	}
}

// Fill out your copyright notice in the Description page of Project Settings.




#include "CXGrapplingHookTriggerBox.h"
#include "CXVI/Character/Field/CXFieldPlayerCharacter.h"
#include "CXVI/Tags/CXFieldTags.h"

#include "AbilitySystemComponent.h"
#include "Components/ShapeComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGrapplingHookTriggerBox) 

ACXGrapplingHookTriggerBox::ACXGrapplingHookTriggerBox(const FObjectInitializer& ObjectInitializer)
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

void ACXGrapplingHookTriggerBox::HandleActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	ACXFieldPlayerCharacter* PlayerCharacter = Cast<ACXFieldPlayerCharacter>(OtherActor);

	// 유효성 검사.
	if (!PlayerCharacter || !IsValid(GrapplingTarget) || !IsValid(LandingSurface))
	{
		return;
	}


	if (UAbilitySystemComponent* ASC = PlayerCharacter->GetAbilitySystemComponent())
	{
		
		ASC->RemoveLooseGameplayTag(CXVIFieldTag::State_CanGrapplingHook);
	
		if (PlayerCharacter->GetGrapplingTargetActor() == GrapplingTarget.Get())
		{
			PlayerCharacter->SetGrapplingTarget(nullptr);
		}
	}
}

void ACXGrapplingHookTriggerBox::HandleActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	ACXFieldPlayerCharacter* PlayerCharacter = Cast<ACXFieldPlayerCharacter>(OtherActor);

	// 유효성 검사.
	if (!PlayerCharacter || !IsValid(GrapplingTarget) || !IsValid(LandingSurface))
	{
		return;
	}

	
	if (UAbilitySystemComponent* ASC = PlayerCharacter->GetAbilitySystemComponent())
	{
		// 태그 변경 보다 먼저 대상 지정.
		PlayerCharacter->SetGrapplingTarget(GrapplingTarget.Get(), LandingSurface.Get());

		ASC->AddLooseGameplayTag(CXVIFieldTag::State_CanGrapplingHook);
	}
}

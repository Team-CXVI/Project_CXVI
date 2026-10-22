// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Character/Combat/CXTempFieldCharacterEnemy.h"

#include "CXVI/Character/Combat/CXTempFieldCharacterPlayer.h"
#include "CXVI/Character/Combat/CXCharacterPartyData.h"

#include "CXVI/Foundation/Combat/CXCombatManagerSubSystem.h"

#include "Components/BoxComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXTempFieldCharacterEnemy)

ACXTempFieldCharacterEnemy::ACXTempFieldCharacterEnemy()
{
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -88.f), FRotator(0.f, -90.f, 0.f));

	// Trigger박스
	EncounterTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	EncounterTrigger->SetupAttachment(RootComponent);
	EncounterTrigger->SetBoxExtent(FVector(100.f, 100.f, 100.f));

	EncounterTrigger->OnComponentBeginOverlap.AddDynamic(this, &ACXTempFieldCharacterEnemy::OnTriggerBeginOverlap);
}

void ACXTempFieldCharacterEnemy::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ACXTempFieldCharacterPlayer* Player = Cast<ACXTempFieldCharacterPlayer>(OtherActor);
	if (!Player)
	{
		return;
	}

	UCXCombatManagerSubsystem* CombatManager = GetWorld()->GetSubsystem<UCXCombatManagerSubsystem>();

	if (!CombatManager)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Player->GetController());
	if (!IsValid(PlayerController))
	{
		return;
	}
	
	CombatManager->RequestStartBattle(Player->GetCharacterData(), PartyData->CharacterArray, PlayerController);
}

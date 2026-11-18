// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CXVI/Character/Combat/CXCombatCharacterBase.h"
#include "CXTempFieldCharacterEnemy.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API ACXTempFieldCharacterEnemy : public ACXCombatCharacterBase
{
	GENERATED_BODY()
	
public:
	ACXTempFieldCharacterEnemy();

protected:
	UFUNCTION()
	void OnTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data", meta = (AllowPrivateAccess = true))
	TObjectPtr<class UCXCharacterPartyData> PartyData;

	UPROPERTY(VisibleAnywhere, Category = "Trigger")
	TObjectPtr<class UBoxComponent> EncounterTrigger;
};

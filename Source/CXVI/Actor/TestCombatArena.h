// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestCombatArena.generated.h"

UCLASS()
class CXVI_API ATestCombatArena : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATestCombatArena();

	const FTransform GetPlayerSlotFromIndex(int32 InIndex) const;
	const FTransform GetEnemySlotFromIndex(int32 InIndex) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slots")
	TArray<TObjectPtr<class UArrowComponent>> PlayerSlots;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slots")
	TArray<TObjectPtr<class UArrowComponent>> EnemySlots;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slots")
	TObjectPtr<class UStaticMeshComponent> BattleArena;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<class UCameraComponent> BattleCamera;

private:
	int32 MaxSlot = 3;

};

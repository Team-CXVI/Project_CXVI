// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterPartyData.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCharacterPartyData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Characters")
	TArray<TSubclassOf<class ACXCombatCharacterBase>> CharacterArray;
};

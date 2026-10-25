// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CXAbilityData.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, Category = "Montage")
	TObjectPtr<class UAnimMontage> AnimMontage;

	UPROPERTY(EditDefaultsOnly, Category = "AnimationSetting")
	FString MontageSectionNamePrefix;

	UPROPERTY(EditDefaultsOnly, Category = "AnimationSetting")
	uint32 MaxComboCount;

	UPROPERTY(EditDefaultsOnly, Category = "AnimationSetting")
	float FrameRate;

};

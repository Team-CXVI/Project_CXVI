// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CXCharacterAbilityData.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXCharacterAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UCXCharacterAbilityData();

	// 섹션 접두사
	UPROPERTY(EditAnywhere, Category = "Name")
	FString MontageSectionNamePrefix;

	// 최대 콤보수
	UPROPERTY(EditAnywhere, Category = "ComboData")
	uint32 MaxComboCount;

	// 프레임 재생속도
	UPROPERTY(EditAnywhere, Category = "ComboData")
	float FrameRate;

	UPROPERTY(EditAnywhere, Category = "ComboData")
	TArray<float> EffectiveFrameCount;
};

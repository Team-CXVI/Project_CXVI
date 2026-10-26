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

	// 타격 시 데미지 배율 몽타주에 이 개수 만큼 애님노티파이를 배치해 피격 이벤트 쏴줘야함
	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	TArray<float> HitDamageMultipilers;

	UPROPERTY(EditDefaultsOnly, Category = "Cost", meta = (ClampMin = "0.0"))
	float ManaCost = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Prefix")
	FString MontageSectionNamePrefix;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|MaxCombo")
	uint32 MaxComboCount;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|FrameRate")
	float FrameRate;

};

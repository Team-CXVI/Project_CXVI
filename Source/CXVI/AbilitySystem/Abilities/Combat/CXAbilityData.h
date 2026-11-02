// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CXAbilityData.generated.h"

UENUM(BlueprintType)
enum class ECXJustTimingType : uint8
{
	None,
	DamageBoost,
	ContinueAttack
};

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

	UPROPERTY(EditDefaultsOnly, Category = "Cost")
	float APCost = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "JustTiming")
	ECXJustTimingType JustTimingType = ECXJustTimingType::None;

	UPROPERTY(EditDefaultsOnly, Category = "JustTiming",
		meta = (ClampMin = "1.0"))
	float JustTimingDamageMultiplier = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "JustTiming")
	bool bJustTimingCannotBeDodged = false;

	// 배열 0번은 1타, 1번은 2타.
	// 항목이 없으면 필요한 판정 수는 1개로 처리.
	UPROPERTY(EditDefaultsOnly, Category = "JustTiming", meta = (ClampMin = "1"))
	TArray<int32> RequiredTimingCounts;

	// 배열 0번은 1타 섹션, 1번은 2타 섹션.
	// 판정 성공 시 다음 배열 항목의 섹션으로 연결.
	UPROPERTY(EditDefaultsOnly, Category = "JustTiming|Combo")
	TArray<FName> AttackSectionNames;

	// None이면 현재 공격 섹션이 끝난뒤 몽타주 종료.
	// 마무리 동작이 있는 스킬은 해당 섹션이름을 지정
	UPROPERTY(EditDefaultsOnly, Category = "JustTiming|Combo")
	FName EndSectionName = NAME_None;

};

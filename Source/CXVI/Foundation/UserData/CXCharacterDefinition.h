// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "CXCharacterDefinition.generated.h"

class ACXCombatCharacterBase;

/**
 * 캐릭터 정의 (파티원 1명)
 * - PrimaryAssetId(CXCharacterDefinition:<에셋 이름>)가 세이브에 저장되는 캐릭터 ID
 * - 스탯 커브 행 이름 = <StatRowPrefix>.<스탯 이름> (예: Warrior.MaxHealth)
 * - Asset Manager 스캔 경로 : /Game/CXVI/Data/Characters (AlwaysCook)
 */
UCLASS(BlueprintType)
class CXVI_API UCXCharacterDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
	FText DisplayName;

	// 전투에서 생성할 캐릭터 BP
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TSoftClassPtr<ACXCombatCharacterBase> CombatCharacterClass;

	// CT_CharacterStat 행 접두사
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stat")
	FName StatRowPrefix;
};

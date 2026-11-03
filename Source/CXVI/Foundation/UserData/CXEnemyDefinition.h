// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CXEnemyDefinition.generated.h"

class ACXCombatCharacterBase;

/**
 * 
 */
UCLASS(BlueprintType)
class CXVI_API UCXEnemyDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
	

public:
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(
        FDataValidationContext& Context
    ) const override;
#endif

    // 화면에 표시할 몬스터 이름
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
    FText DisplayName;

    // 전투 아레나에서 생성할 적 캐릭터 클래스
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    TSoftClassPtr<ACXCombatCharacterBase> CombatCharacterClass;

    // 몬스터 스탯 커브 행을 찾을 때 사용할 접두사
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stat")
    FName StatRowPrefix = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EXP")
    int32 Exp;
};

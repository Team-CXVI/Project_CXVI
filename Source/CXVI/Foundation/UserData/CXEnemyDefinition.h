// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CXEnemyDefinition.generated.h"

class ACXCombatCharacterBase;
class UTexture2D;

USTRUCT(BlueprintType)
struct FCXEnemySkillChoice
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Skill")
    FName SkillName = NAME_None;

    UPROPERTY(EditAnywhere, Category = "Skill", meta = (ClampMin = "0"))
    int32 Weight = 1;
};

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

    // UI에 표시할 초상화 (필드 HUD 파티 상태)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSoftObjectPtr<UTexture2D> Portrait;

    // 전투 아레나에서 생성할 적 캐릭터 클래스
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    TSoftClassPtr<ACXCombatCharacterBase> CombatCharacterClass;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Skill")
    FName SkillRowPrefix = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Skill")
    TArray<FCXEnemySkillChoice> Skills;

    // 몬스터 스탯 커브 행을 찾을 때 사용할 접두사
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stat")
    FName StatRowPrefix = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
    bool bIsBoss = false;
};

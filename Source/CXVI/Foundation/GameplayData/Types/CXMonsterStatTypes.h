#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CXMonsterStatTypes.generated.h"

USTRUCT(BlueprintType)
struct CXVI_API FCXMonsterStatRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster Stat")
    float MaxHealth = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster Stat")
    float CombatSpeed = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster Stat")
    float AttackPower = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster Stat")
    float Defense = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster Stat")
    float CriticalChance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster Stat")
    float CriticalDamageMultiplier = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster Stat")
    float DodgeChance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
    int32 Exp = 0;
};
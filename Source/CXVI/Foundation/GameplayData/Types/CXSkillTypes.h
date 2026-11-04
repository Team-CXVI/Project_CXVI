#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CXSkillTypes.generated.h"

class UCXGameplayAbilityCombatBase;

USTRUCT(BlueprintType)
struct CXVI_API FCXSkillRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
    TSoftClassPtr<UCXGameplayAbilityCombatBase> AbilityClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill",
        meta = (MultiLine = "true"))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill",
        meta = (ClampMin = "0.0"))
    float APCost = 0.f;
};
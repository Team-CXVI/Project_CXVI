#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CXEncounterTypes.generated.h"

class UCXEnemyDefinition;

USTRUCT(BlueprintType)
struct CXVI_API FCXEncounterRow : public FTableRowBase
{
    GENERATED_BODY()

    // 배열 순서대로 배치할 몬스터 정의
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
    TArray<FPrimaryAssetId> Enemies;
};
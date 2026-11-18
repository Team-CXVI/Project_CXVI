#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CXEncounterTypes.generated.h"

class UCXEnemyDefinition;
class UCXSceneDefinition;

USTRUCT(BlueprintType)
struct CXVI_API FCXEncounterRow : public FTableRowBase
{
    GENERATED_BODY()

    // 배열 순서대로 배치할 몬스터 정의
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
    TArray<FPrimaryAssetId> Enemies;

    // 이 인카운터에서 사용할 전투 Scene (보스 전용 전투 레벨 등)
    // - None이면 현재 레벨 정의(UCXLevelDefinition)의 CombatScene 사용
    // - 이 Scene의 Sublevels는 이 인카운터가 나오는 필드 레벨(퍼시스턴트)의 Levels 창에 등록되어 있어야 함
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
    TObjectPtr<UCXSceneDefinition> CombatScene;
};
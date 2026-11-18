// Fill out your copyright notice in the Description page of Project Settings.


#include "Foundation/UserData/CXEnemyDefinition.h"
//
//#include "Character/Combat/CXCombatCharacterBase.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXEnemyDefinition)

#if WITH_EDITOR
EDataValidationResult UCXEnemyDefinition::IsDataValid(
    FDataValidationContext& Context) const
{
    bool bIsValid = Super::IsDataValid(Context)
        != EDataValidationResult::Invalid;

    if (CombatCharacterClass.IsNull())
    {
        Context.AddError(
            FText::FromString(TEXT("전투 몬스터 클래스가 지정되지 않았습니다."))
        );
        bIsValid = false;
    }

    if (StatRowPrefix.IsNone())
    {
        Context.AddError(
            FText::FromString(TEXT("몬스터 스탯 행 접두사가 비어 있습니다."))
        );
        bIsValid = false;
    }

    return bIsValid
        ? EDataValidationResult::Valid
        : EDataValidationResult::Invalid;
}
#endif

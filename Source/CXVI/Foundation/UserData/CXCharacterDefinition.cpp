// Fill out your copyright notice in the Description page of Project Settings.


#include "Foundation/UserData/CXCharacterDefinition.h"

#if WITH_EDITOR
#include <Misc/DataValidation.h>
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCharacterDefinition)

#if WITH_EDITOR
EDataValidationResult UCXCharacterDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (CombatCharacterClass.IsNull())
	{
		Context.AddError(FText::FromString(TEXT("CombatCharacterClass가 지정되지 않았습니다.")));
		Result = EDataValidationResult::Invalid;
	}

	if (StatRowPrefix.IsNone())
	{
		Context.AddError(FText::FromString(TEXT("StatRowPrefix가 비어 있습니다. (스탯 커브 행 이름에 사용)")));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

// Fill out your copyright notice in the Description page of Project Settings.


#include "Level/CXLevelDefinition.h"

#include "Foundation/Scene/CXSceneDefinition.h"

#if WITH_EDITOR
#include <Misc/DataValidation.h>
#endif //WITH_EDITOR

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXLevelDefinition)

#if WITH_EDITOR
EDataValidationResult UCXLevelDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (Level.IsNull())
	{
		Context.AddError(FText::FromString(TEXT("Level이 지정되지 않았습니다.")));
		Result = EDataValidationResult::Invalid;
	}

	if (ECXLevelType::InGame == LevelType && nullptr == InitialScene)
	{
		Context.AddError(FText::FromString(TEXT("InGame 레벨은 InitialScene이 필요합니다.")));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif //WITH_EDITOR

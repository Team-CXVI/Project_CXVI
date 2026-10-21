// Fill out your copyright notice in the Description page of Project Settings.


#include "CXSceneDefinition.h"

#if WITH_EDITOR
EDataValidationResult UCXSceneDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	// TODO : 데이터 무결성 확인
	return Result;
}
#endif

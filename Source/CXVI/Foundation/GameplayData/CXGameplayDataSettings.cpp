// Fill out your copyright notice in the Description page of Project Settings.


#include "CXGameplayDataSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGameplayDataSettings)

UCXGameplayDataSettings::UCXGameplayDataSettings(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	// Project Settings 왼쪽 목록에서 "Game" 그룹 아래 표시
	CategoryName = TEXT("Game");
}

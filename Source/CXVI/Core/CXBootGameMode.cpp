// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/CXBootGameMode.h"

#include "Level/CXLevelTransitionSubsystem.h"
#include "Level/CXLevelTransitionSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXBootGameMode)

ACXBootGameMode::ACXBootGameMode()
{
	// Boot 레벨이기 때문에 Pawn이 필요 없음
	DefaultPawnClass = nullptr;
}

void ACXBootGameMode::BeginPlay()
{
	Super::BeginPlay();

	UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this);
	if (nullptr == LevelTransition || !LevelTransition->TravelToTitle())
	{
		UE_LOG(LogCXLevelTransition, Error, TEXT("부트 맵에서 타이틀 레벨로 이동하지 못했습니다."));
	}
}

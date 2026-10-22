// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/CXGameMode.h"
#include "Foundation/Scene/CXSceneDefinition.h"
#include "Foundation/Scene/CXSceneSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGameMode)

void ACXGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	if (InitialScene)
	{
		UCXSceneSubsystem::Get(this).PushScene(InitialScene, FCXSceneContext());
	}
}

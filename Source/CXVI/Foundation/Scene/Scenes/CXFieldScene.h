// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Foundation/Scene/CXGameScene.h"

#include "CXFieldScene.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXFieldScene : public UCXGameScene
{
	GENERATED_BODY()
	
protected:
	virtual void OnResume(const FCXSceneContext& InContext) override;

	virtual void OnSuspend() override;

	virtual void OnExit() override;

	virtual void SetupSceneInput() override;
};

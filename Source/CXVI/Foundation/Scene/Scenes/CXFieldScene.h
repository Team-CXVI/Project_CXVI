// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Foundation/Scene/CXGameScene.h"

#include "CXFieldScene.generated.h"

class UCXSceneDefinition;

/**
 * 
 */
UCLASS()
class CXVI_API UCXFieldScene : public UCXGameScene
{
	GENERATED_BODY()
	
public:
	// 인카운터 발생 시 호출.
	UFUNCTION(BlueprintCallable, Category = "CXVI|Field")
	bool RequestCombat(UCXSceneDefinition* InCombatScene, FName InEncounterId);
	
protected:
	virtual void OnResume(const FCXSceneContext& InContext) override;
};

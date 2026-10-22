// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CXVI/Foundation/Scene/CXGameScene.h"

#include "CXCombatScene.generated.h"

// TODO : 임시 구조체. 추후에 데이터가 정해지면 수정 필요
// 필드 -> 전투 (FCXSceneContext::Payload)

USTRUCT(BlueprintType)
struct CXVI_API FCXCombatEnterParams
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FName EncounterId;
};

// TODO : 임시 구조체. 추후에 데이터가 정해지면 수정 필요
/** 전투 → 필드 (PopScene의 ResultContext::Payload) */
USTRUCT(BlueprintType)
struct CXVI_API FCXCombatResult
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bVictory = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 EarnedExp = 0;
};


/**
 * 전투 Scene
 * - PlayerController는 교체하지 않음. ViewTarget만 전투 카메라로 교체
 * - 전투 플레이어 및 몬스터는 SpawnSceneActor로 스폰 -> Exit 시 자동 Destory
 */
UCLASS()
class CXVI_API UCXCombatScene : public UCXGameScene
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "CXVI|Combat")
	void EndCombat(const FCXCombatResult& InResult);

protected:
	virtual void OnEnter(const FCXSceneContext& InContext) override;

	virtual void OnExit() override;

private:
	void SetupCombatCamera();

	void SpawnCombatants(const FCXCombatEnterParams& InParams);
};

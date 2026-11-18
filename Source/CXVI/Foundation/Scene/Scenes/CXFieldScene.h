// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Foundation/Scene/CXGameScene.h"

#include "CXFieldScene.generated.h"

struct FCXCombatResult;

/**
 * 필드 Scene
 * - 전투에서 돌아오면(OnResume, Payload = FCXCombatResult) 결과를 필드에 반영
 *   - 승리 : 전투를 건 필드 적을 처치 처리
 *   - 보스 처치(bBossDefeated) : 클리어 화면(UCXUISettings::GameClearClass)을 UI.Layer.GameMenu에 Push
 *     OnResume은 FadeIn 전이라 화면이 밝아지면서 함께 보임
 *   - 보스전이 아닌 승리 : 처치 반영 후 자동 저장 요청 (Message.UserData.RequestAutoSave)
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

private:
	// 전투를 건 필드 적의 처치 처리 (승리일 때만. 전투에 들어간 SpawnId는 결과와 관계없이 비움)
	void DefeatCombatEnemy(const FCXCombatResult& InResult);

	// 인카운터에 보스 적 정의(bIsBoss)가 있는지
	bool IsBossEncounter(FName InEncounterId) const;

	// 보스 클리어 화면을 UI.Layer.GameMenu에 Push
	void ShowGameClearScreen();
};

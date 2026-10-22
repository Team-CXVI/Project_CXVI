// Fill out your copyright notice in the Description page of Project Settings.


#include "CXFieldScene.h"
#include "CXCombatScene.h"
#include "CXVI/Foundation/Scene/CXSceneSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldScene)


bool UCXFieldScene::RequestCombat(UCXSceneDefinition* InCombatScene, FName InEncounterId)
{
	UCXSceneSubsystem* Subsystem = GetSceneSubsystem();
	if (!Subsystem)
	{
		return false;
	}

	FCXCombatEnterParams Params;
	Params.EncounterId = InEncounterId;

	return Subsystem->PushScene(InCombatScene, FCXSceneContext::Make(Params));
}

void UCXFieldScene::OnResume(const FCXSceneContext& InContext)
{
	Super::OnResume(InContext);

}

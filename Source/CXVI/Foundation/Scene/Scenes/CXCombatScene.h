// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CXVI/Foundation/Scene/CXGameScene.h"
#include "CXVI/Foundation/Combat/CXCombatTypes.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"

#include "CXCombatScene.generated.h"

class ACXCombatArena;
class UCXCombatManagerSubsystem_V2;
struct FCXCombatEndMessage;

class UCXCombatViewModel;

/**
* 전투 Scene
* - 진입은 UCXCombatManagerSubsystem::RequestStartBattle이 Push(Payload = FCXCombatEnterParams)
* - 커맨드 Pawn은 Definition의 PawnClass로 베이스가 찾아서 빙의(전투 레벨에 배치)
* - Enter : Message.Combat.End 구독 -> 아레나에 전투원 생성 요청 -> 전투 매니저에 BeginBattle
* - 종료 메시지를 받으면 Pop->Exit에서 턴 정지 + 전투원 정리(화면이 가려진 뒤)
* - 진입 전환이 끝나기 전에 종료되면(시작 실패 포함) 전환이 끝난 뒤 Pop
* - HUD : Definition의 HUDClass(UCXCombatHUDWidget)에 CombatViewModel 주입. 
*	전투원 생성 후 SetCombatants, Exit에서 전투원 정리 전에 Reset
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

	virtual void InitializeSceneHUD(UCXActivatableWidget& InHUD) override;

private:
	// 아레나 검색 -> 전투원 생성 -> 전투 매니저에 전달. 하나라도 실패하면 false
	bool StartBattle(const FCXSceneContext& InContext);

	void HandleCombatEnded(const FGameplayTag& InChannel, const FCXCombatEndMessage& InMessage);

	// 전환 중에 들어온 종료 요청을 전환이 끝난 뒤 처리
	void HandleSceneChanged(UCXGameScene* InNewScene, UCXGameScene* InOldScene);

	void UnbindSceneChanged();

	UCXCombatManagerSubsystem_V2* GetCombatManager() const;

private:
	FCXGameplayMessageListenerHandle CombatEndListener;

	FDelegateHandle SceneChangedHandle;

	FCXCombatResult PendingResult;

	// 전투원을 생성한 아레나 (Exit에서 정리)
	TWeakObjectPtr<ACXCombatArena> Arena;

	// 전투 HUD 표시 데이터 (Enter에서 생성, HUD가 Resume으로 다시 생성되어도 유지)
	UPROPERTY()
	TObjectPtr<UCXCombatViewModel> CombatViewModel;
};

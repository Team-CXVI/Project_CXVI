// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "CXGameMode.generated.h"

class UCXGameScene;

/**
* InGame 레벨의 GameMode(모든 InGame 레벨이 이 클래스 혹은 상속해서 사용)
* - 첫 Scene의 서브레벨은 레벨 열기 과정(플레이어 스폰 전)에서 이미 로드되어 있음
* - 스폰 위치는 레벨 전환의 진입 태그와 PlayerStartTag가 같은 PlayerStart
* - BeginPlay에서 첫 Scene을 화면 제어 없이 Push (레벨 전환 구간의 화면은 레벨 전환이 담당)
* - 첫 Scene 전환이 끝나면 레벨 전환에 준비 완료를 알림
*/
UCLASS()
class CXVI_API ACXGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
protected:
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleInitialSceneReady(UCXGameScene* InNewScene, UCXGameScene* InOldScene);

	void UnbindSceneDelegates();

	// 레벨 전환이 완료(InitialScene 로드가 완료)되었다는 통지
	void NotifyLevelReady();

public:
	FDelegateHandle SceneChangedHandle;

};

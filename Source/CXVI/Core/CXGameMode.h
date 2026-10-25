// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CXGameMode.generated.h"

class UCXGameScene;

/**
* InGame 레벨의 GameMode(모든 InGame 레벨이 이 클래스 혹은 상속해서 사용)
* -현재 레벨 정의의 InitialScene을 Push
* -PlayerStart는 필드 서브레벨에 있으므로, 첫 Scene의 레벨이 Visible이 된 뒤(카메라 Fade In 전)에 플레이어를 스폰하는 처리를 진행
* -스폰 위치는 레벨 전환의 진입 태그와 PlayerStartTag가 같은 PlayerStart
* -첫 Scene 전환이 끝나면 레벨 전환에 준비 완료를 알림
*/
UCLASS()
class CXVI_API ACXGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// PostLogin 직후 플레이어 Pawn을 스폰하지 않고 첫 Scene 진입까지 보류
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

private:
	void HandleInitialSceneEntered(UCXGameScene* InEnteredScene);

	void HandleInitialSceneReady(UCXGameScene* InNewScene, UCXGameScene* InOldScene);

	// 스폰을 보류한 플레이어를 진입 태그의 PlayerStart에 스폰
	void SpawnPendingPlayers();

	void UnbindSceneDelegates();

	// 레벨 전환이 완료(InitialScene 로드가 완료)되었다는 통지
	void NotifyLevelReady();

public:
	
	// PlayerController 지연 생성을 위한 임시 저장
	TArray<TWeakObjectPtr<APlayerController>> PendingStartPlayers;

	// 첫 Scene 진입 전까지 true (이 때 로그인한 플레이어는 스폰 보류)
	bool bWaitingForInitialScene = true;

	FDelegateHandle SceneEnteredHandle;

	FDelegateHandle SceneChangedHandle;
};

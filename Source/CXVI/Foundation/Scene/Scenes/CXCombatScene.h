// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CXVI/Foundation/Scene/CXGameScene.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"

#include "CXCombatScene.generated.h"

class ACXCombatCharacterBase;
class UCXCombatManagerSubsystem;
struct FCXCombatEndMessage;

// TODO : 임시 구조체. 추후에 데이터가 정해지면 수정 필요
// 필드 -> 전투 (FCXSceneContext::Payload)

USTRUCT(BlueprintType)
struct CXVI_API FCXCombatEnterParams
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FName EncounterId;

	// TODO : 임시. 파티 정보 출처가 정해지면 EncounterId 조회 등으로 변경
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<TSubclassOf<ACXCombatCharacterBase>> PlayerClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<TSubclassOf<ACXCombatCharacterBase>> EnemyClasses;
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
 * - 커맨드 Pawn은 Definition의 PawnClass로 베이스가 찾아서 빙의 (전투 레벨에 배치)
 * - Enter : Message.Combat.End 구독 + 전투 매니저에 Scene 전투 시작 요청
 * - 종료 메시지를 받으면 Pop -> Exit에서 전투원 정리 (화면이 가려진 뒤)
 * - 진입 전환이 끝나기 전에 종료되면(시작 실패 포함) 전환이 끝난 뒤 Pop
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

private:
	void HandleCombatEnded(const FGameplayTag& InChannel, const FCXCombatEndMessage& InMessage);

	// 전환 중에 들어온 종료 요청을 전환이 끝난 뒤 처리
	void HandleSceneChanged(UCXGameScene* InNewScene, UCXGameScene* InOldScene);

	void UnbindSceneChanged();

	UCXCombatManagerSubsystem* GetCombatManager() const;

private:
	FCXGameplayMessageListenerHandle CombatEndListener;

	FDelegateHandle SceneChangedHandle;

	FCXCombatResult PendingResult;

};

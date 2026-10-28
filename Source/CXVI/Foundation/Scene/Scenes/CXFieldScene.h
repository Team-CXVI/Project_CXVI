// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Foundation/Scene/CXGameScene.h"

#include "CXFieldScene.generated.h"

class ACharacter;
class UCXSceneDefinition;
struct FInputActionValue;

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

	virtual void OnSuspend() override;

	virtual void OnExit() override;

	virtual void SetupSceneInput() override;

private:
	// TODO : 임시. 일시정지 메뉴가 생기면 제거 (ESC -> 타이틀 이동)
	void HandleEscapeKey(FKey InKey, FInputActionValue InValue);

	void ClearEscapeBinding();

	// 디버그 키 바인딩 핸들 (베이스의 Scene 입력 핸들과 별도로 해제)
	uint32 EscapeBindingHandle = 0;
};

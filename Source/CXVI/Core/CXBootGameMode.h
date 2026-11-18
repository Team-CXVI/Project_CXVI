// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "CXBootGameMode.generated.h"

/**
 * 부트 레벨(Game Default Map)의 GameMode
 * - 실행 직후 Settings의 TitleLevel로 이동
 * - 나중에 스플래시 / 로고 화면을 넣을 자리
 */
UCLASS()
class CXVI_API ACXBootGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACXBootGameMode();

protected:
	virtual void BeginPlay() override;
};

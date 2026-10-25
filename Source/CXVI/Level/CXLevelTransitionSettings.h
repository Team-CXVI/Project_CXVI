// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "CXLevelTransitionSettings.generated.h"

/**
 * 레벨 전환 설정
 * - 프로젝트에 하나뿐인 진입점 레벨 (레벨 목록이 아님)
 * Project Settings > Game > CXVI Level Transition
 * 저장 위치 : Config/DefaultGame.ini [/Script/CXVI.CXLevelTransitionSettings]
 */

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "CXVI Level Transition"))
class CXVI_API UCXLevelTransitionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UCXLevelTransitionSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	// 타이틀 레벨 (TravelToTitle의 목적지)
	UPROPERTY(Config, EditAnywhere, Category = "Level", meta = (AllowedTypes = "CXLevelDefinition"))
	FPrimaryAssetId TitleLevel;

	// 새 게임을 시작하는 첫 인게임 레벨 (타이틀 화면의 "게임 시작")
	UPROPERTY(Config, EditAnywhere, Category = "Level", meta = (AllowedTypes = "CXLevelDefinition"))
	FPrimaryAssetId NewGameLevel;
};

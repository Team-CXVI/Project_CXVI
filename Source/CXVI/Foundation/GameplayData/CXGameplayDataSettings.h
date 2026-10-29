// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "CXGameplayDataSettings.generated.h"

class UDataTable;
class UCurveTable;
class UCXNewGameData;

/**
 * 게임플레이 데이터 테이블 경로 설정
 * Project Settings > Game > CXVI Gameplay Data
 * 저장 위치 : Config/DefaultGame.ini [/Script/CXVI.CXGameplayDataSettings]
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "CXVI Gameplay Data"))
class CXVI_API UCXGameplayDataSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UCXGameplayDataSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
public:
	UPROPERTY(Config, EditAnywhere, Category = "Item", meta = (RequiredAssetDataTags = "RowStructure=/Script/CXVI.CXItemRow"))
	TSoftObjectPtr<UDataTable> ItemTable;

	UPROPERTY(Config, EditAnywhere, Category = "Stat")
	TSoftObjectPtr<UCurveTable> CharacterStatCurve;

	UPROPERTY(Config, EditAnywhere, Category = "Stat")
	TSoftObjectPtr<UCurveTable> MonsterStatCurve;

	// 새 게임 시작 파티 / 아이템
	UPROPERTY(Config, EditAnywhere, Category = "UserData")
	TSoftObjectPtr<UCXNewGameData> NewGameData;

	// 수동 저장 슬롯 수 (슬롯 0은 자동 저장 전용)
	UPROPERTY(Config, EditAnywhere, Category = "Save", meta = (ClampMin = 1))
	int32 MaxManualSaveSlots = 3;
};

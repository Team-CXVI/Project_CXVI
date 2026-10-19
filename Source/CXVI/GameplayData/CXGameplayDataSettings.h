// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CXGameplayDataSettings.generated.h"

class UDataTable;
class UCurveTabel;

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
};

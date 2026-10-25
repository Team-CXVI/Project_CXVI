// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "CXLevelDefinition.generated.h"

class UCXSceneDefinition;

/** 레벨 타입 */
UENUM(BlueprintType)
enum class ECXLevelType : uint8
{
	Title,

	InGame
};

/**
 * 레벨 정의
 *  - 레벨 하나에 대한 데이터 (레벨, 도착 시 Push할 첫 Scene, 이 레벨로 이동할 때의 페이드 설정)
 *  - PrimaryAssetId는 엔진 기본 규칙을 따름 (타입 = CXLevelDefinition, 이름 = 에셋 이름)
 *  - Asset Manager 스캔 경로 : /Game/CXVI/Data/Levels
 */
UCLASS(BlueprintType)
class CXVI_API UCXLevelDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif //WITH_EDITOR

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Level")
	ECXLevelType LevelType = ECXLevelType::InGame;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Level")
	TSoftObjectPtr<UWorld> Level;

	// 첫 Scene. 레벨을 열 때 이 Scene의 서브레벨이 플레이어 스폰 전에 동기로 로드되고, 도착 후 Push됨 (InGame은 필수 설정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scene")
	TObjectPtr<UCXSceneDefinition> InitialScene;
	
	// 이 레벨로 이동할 때의 페이드 아웃 시간. 0이면 즉시 검은 화면으로 전환
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = 0.0, Units = "s"))
	float FadeOutDuration = 0.5f;

	// 이 레벨에 준비가 끝난 뒤 화면을 여는 페이드 인 시간. 0이면 즉시 공개
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = 0.0, Units = "s"))
	float FadeInDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transition")
	FLinearColor FadeColor = FLinearColor::Black;

	//TODO : TEMP 이 레벨에서 사용할 전투 Scene (인카운터 트리거가 생기면 정리)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scene|Test")
	TObjectPtr<UCXSceneDefinition> CombatScene;

};

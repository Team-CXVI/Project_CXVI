// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CXSceneTypes.h"

#include "CXSceneDefinition.generated.h"

class UCXGameScene;

/**
 * 
 */
UCLASS(BlueprintType)
class CXVI_API UCXSceneDefinition : public UDataAsset
{
	GENERATED_BODY()
	
public:
#if WITH_EDITOR
	// 데이터 무결성 검사
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

public:
	// 이 Scene이 Push될 때 생성할 Scene 오브젝트
	UPROPERTY(EditDefaultsOnly, Category = "Scene")
	TSubclassOf<UCXGameScene> SceneClass;
	
	// Persistant Level 창에 등록된 SubLevel (Streaming Method = Blueprint, 초기 언로드)
	// [0]이 Primary Level (SpawnSceneActor의 Spawn 대상)
	// 다른 Scene 정의와 공유해도 됨 (상태는 참조 기반으로 계산)
	UPROPERTY(EditDefaultsOnly, Category = "Level")
	TArray<TSoftObjectPtr<UWorld>> Sublevels;

	UPROPERTY(EditDefaultsOnly, Category = "Level")
	ECXSceneExitPolicy ExitPolicy = ECXSceneExitPolicy::Unload;

	// 이 Scene이 Push 될 때 Hidden 상태로 미리 로드할 Scene(필드 -> 현재 지역의 전투 레벨)
	UPROPERTY(EditDefaultsOnly, Category = "Level")
	TArray<TObjectPtr<UCXSceneDefinition>> PreloadScenes;

	// 해당 Scene에 바인딩 할 InputMappingContext
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TArray<FCXSceneInputMapping> IMCs;

	UPROPERTY(EditDefaultsOnly, Category = "Transition", meta = (ClampMin = 0.0, Units = "s"))
	float FadeOutDuration = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Transition", meta = (ClampMin = 0.0, Units = "s"))
	float FadeInDuration = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Transition")
	FLinearColor FadeColor = FLinearColor::Black;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "Foundation/Combat/CXCombatTypes.h"
#include "CXCombatMenuCameraData.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXCombatMenuCameraData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	virtual void PostLoad() override;

	UPROPERTY(EditAnywhere, Category = "Camera")
	TMap<FName, FCXMenuCameraPreset> CameraPresets;

	UPROPERTY(EditAnywhere, Category = "Camera")
	TMap<ECommandMenuState, FName> MenuCameraIds;

private:
	// 이전 DataAsset에 저장된 메뉴별 구도를 불러오기 위한 필드.
	UPROPERTY()
	TMap<ECommandMenuState, FCXMenuCameraPreset> MenuPresets;

};

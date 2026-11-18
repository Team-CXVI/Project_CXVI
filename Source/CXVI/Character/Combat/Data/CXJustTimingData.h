// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CXJustTimingData.generated.h"

USTRUCT()
struct FCXJustTimingSettings
{
    GENERATED_BODY()

    // 입력을 받는 전체 시간
    UPROPERTY(EditAnywhere, Category = "Just Timing", meta = (ClampMin = "0.01"))
    float WindowDurationSeconds = 0.5f;

    // 전체 시간에서 성공 구간이 시작되는 비율
    UPROPERTY(EditAnywhere, Category = "Just Timing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SuccessStartRatio = 0.75f;

    // 전체 시간에서 성공 구간이 끝나는 비율
    UPROPERTY(EditAnywhere, Category = "Just Timing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SuccessEndRatio = 1.0f;
};

/**
 * 
 */
UCLASS(BlueprintType)
class CXVI_API UCXJustTimingData : public UDataAsset
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditAnywhere, Category = "Just Timing")
    FCXJustTimingSettings Settings;
};

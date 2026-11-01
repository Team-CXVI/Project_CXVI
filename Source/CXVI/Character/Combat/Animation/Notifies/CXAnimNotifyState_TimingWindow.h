// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "CXAnimNotifyState_TimingWindow.generated.h"

/**
 * 
 */
UCLASS(meta = (DisplayName = "CX Just Timing Window"))
class CXVI_API UCXAnimNotifyState_TimingWindow : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:
    virtual void NotifyBegin(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        float TotalDuration,
        const FAnimNotifyEventReference& EventReference) override;

    virtual void NotifyEnd(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

private:
    UPROPERTY(EditAnywhere, Category = "JustTiming", meta = (ClampMin = "1"))
    int32 HitNumber = 1;

    UPROPERTY(EditAnywhere, Category = "JustTiming", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SuccessStartRatio = 0.7f;

    UPROPERTY(EditAnywhere, Category = "JustTiming", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SuccessEndRatio = 0.9f;
};

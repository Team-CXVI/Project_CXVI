// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "CXAnimNotify_JustTimingStart.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXAnimNotify_JustTimingStart : public UAnimNotify
{
	GENERATED_BODY()
	

public:
    UCXAnimNotify_JustTimingStart(const FObjectInitializer& ObjectInitializer);

    virtual void Notify(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

private:
    UPROPERTY(EditAnywhere, Category = "JustTiming", meta = (ClampMin = "1"))
    int32 HitNumber = 1;

    UPROPERTY(EditAnywhere, Category = "JustTiming", meta = (ClampMin = "1"))
    int32 TimingIndex = 1;
};

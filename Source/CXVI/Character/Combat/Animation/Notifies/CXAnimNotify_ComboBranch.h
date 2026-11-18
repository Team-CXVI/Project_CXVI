// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "CXAnimNotify_ComboBranch.generated.h"

/**
 * 
 */
UCLASS(meta = (DisplayName = "CX Combo Branch"))
class CXVI_API UCXAnimNotify_ComboBranch : public UAnimNotify
{
	GENERATED_BODY()
	
public:
    virtual void Notify(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

private:
    // 1이면 Attack1의 판정을 확인해 Attack2 연결 여부 결정.
    // 데미지 타격 번호와 별개인 공격 동작 번호.
    UPROPERTY(EditAnywhere, Category = "JustTiming", meta = (ClampMin = "1"))
    int32 StageNumber = 1;
};

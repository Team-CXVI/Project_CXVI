// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "CXAnimNotify_AttackHit.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXAnimNotify_AttackHit : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation, const
		FAnimNotifyEventReference& EventReference
	) override;

private:
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "1"))
	int32 ComboIndex = 1;

	
};

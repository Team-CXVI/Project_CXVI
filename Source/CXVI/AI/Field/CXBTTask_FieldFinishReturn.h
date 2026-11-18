// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "CXBTTask_FieldFinishReturn.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXBTTask_FieldFinishReturn : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UCXBTTask_FieldFinishReturn();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
};

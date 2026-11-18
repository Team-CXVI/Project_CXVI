// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "CXBTTask_FieldAttack.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXBTTask_FieldAttack : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UCXBTTask_FieldAttack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

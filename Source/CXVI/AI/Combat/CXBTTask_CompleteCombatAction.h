// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "CXBTTask_CompleteCombatAction.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXBTTask_CompleteCombatAction : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UCXBTTask_CompleteCombatAction();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

};

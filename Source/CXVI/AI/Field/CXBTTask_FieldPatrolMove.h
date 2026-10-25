// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "CXBTTask_FieldPatrolMove.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXBTTask_FieldPatrolMove : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UCXBTTask_FieldPatrolMove();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual uint16 GetInstanceMemorySize() const override;


protected:
	UPROPERTY(EditAnywhere, Category = "Patrol")
	float AcceptanceRadius = 5.0f;
	
};

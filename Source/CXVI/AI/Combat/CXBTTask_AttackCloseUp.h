// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "CXBTTask_AttackCloseUp.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXBTTask_AttackCloseUp : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UCXBTTask_AttackCloseUp();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	UPROPERTY(EditAnywhere, Category = "Camera")
	bool bUseAttackLocation = false;
};

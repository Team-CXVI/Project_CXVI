// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FieldFindPatrolPos.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UBTTask_FieldFindPatrolPos : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_FieldFindPatrolPos();

protected:
	// 태스크 실행을 시작할 때 호출되는 함수.
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

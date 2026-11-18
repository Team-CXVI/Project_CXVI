// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "CXBTDecorator_FieldAttackInRange.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXBTDecorator_FieldAttackInRange : public UBTDecorator
{
	GENERATED_BODY()
	
public:
	UCXBTDecorator_FieldAttackInRange();

protected:
	// 데코레이터의 조건을 평가할 때 실행되는 함수 오버라이드.
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

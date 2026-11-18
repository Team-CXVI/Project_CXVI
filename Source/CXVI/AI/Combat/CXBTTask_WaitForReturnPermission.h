// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "CXBTTask_WaitForReturnPermission.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXBTTask_WaitForReturnPermission : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UCXBTTask_WaitForReturnPermission();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	void HandleCanReturn(bool bWasCancelled);
	void Cleanup();

private:
	TWeakObjectPtr<class ACXCombatCharacterBase> CombatCharacter;
	FDelegateHandle CanReturnHandle;

	bool bCanReturn = false;
};

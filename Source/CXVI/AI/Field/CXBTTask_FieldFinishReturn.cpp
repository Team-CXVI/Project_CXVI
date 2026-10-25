// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Field/CXBTTask_FieldFinishReturn.h"
#include "CXFieldAI.h"

#include "BehaviorTree/BlackboardComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXBTTask_FieldFinishReturn)

UCXBTTask_FieldFinishReturn::UCXBTTask_FieldFinishReturn()
{
	NodeName = TEXT("Finish Return");
}

EBTNodeResult::Type UCXBTTask_FieldFinishReturn::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	// 이전 추적 대상과 정차 목적지를 정리.
	Blackboard->ClearValue(BBKEY_TARGET);
	Blackboard->ClearValue(BBKEY_PATROLPOS);

	// 복귀 완료. 정찰 감지 다시 허용.
	Blackboard->SetValueAsBool(BBKEY_ISRETURNING, false);

	UE_LOG(LogTemp, Display, TEXT("[Return] %s: 복귀 완료"), *OwnerComp.GetPathName());


	return EBTNodeResult::Succeeded;
}

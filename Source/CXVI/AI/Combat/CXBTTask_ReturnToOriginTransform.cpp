// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Combat/CXBTTask_ReturnToOriginTransform.h"

#include "Character/Combat/CXCombatCharacterBase.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXBTTask_ReturnToOriginTransform)

UCXBTTask_ReturnToOriginTransform::UCXBTTask_ReturnToOriginTransform()
{
	NodeName = TEXT("Return To Origin Transform");
}

EBTNodeResult::Type UCXBTTask_ReturnToOriginTransform::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();

	ACXCombatCharacterBase* Character = AIController ? Cast<ACXCombatCharacterBase>(AIController->GetPawn()) : nullptr;

	if (!IsValid(Character))
	{
		return EBTNodeResult::Failed;
	}

	Character->ReturnToOriginTransform();

	return EBTNodeResult::Succeeded;
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Combat/CXBTTask_CompleteCombatAction.h"

#include "Character/Combat/CXCombatCharacterBase.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(CXBTTask_CompleteCombatAction)

UCXBTTask_CompleteCombatAction::UCXBTTask_CompleteCombatAction()
{
	NodeName = TEXT("Complete Combat Action");
}

EBTNodeResult::Type UCXBTTask_CompleteCombatAction::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();

	ACXCombatCharacterBase* Character = AIController ? Cast<ACXCombatCharacterBase>(AIController->GetPawn()) : nullptr;

	if (!IsValid(Character))
	{
		return EBTNodeResult::Failed;
	}

	Character->CompleteCombatBehavior();

	return EBTNodeResult::Succeeded;
}

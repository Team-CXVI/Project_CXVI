// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Combat/CXBTTask_AttackCloseUp.h"

#include "Foundation/Combat/CXCombatManagerSubsystem.h"
#include "Character/Combat/CXCombatCharacterPlayer.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXBTTask_AttackCloseUp)

UCXBTTask_AttackCloseUp::UCXBTTask_AttackCloseUp()
{
	NodeName = TEXT("Camera Close Up");
}

EBTNodeResult::Type UCXBTTask_AttackCloseUp::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();

	ACXCombatCharacterPlayer* Character = AIController ? Cast<ACXCombatCharacterPlayer>(AIController->GetPawn()) : nullptr;

	if (!IsValid(Character))
	{
		return EBTNodeResult::Failed;
	}

	if (!Character->GetIsTurn())
	{
		return EBTNodeResult::Succeeded;
	}

	if (const UCXCombatManagerSubsystem* Manager =
		UCXCombatManagerSubsystem::TryGet(Character))
	{
		TOptional<FVector> AnchorLocationOverride;
		const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
		const FName AttackLocationKey(TEXT("AttackLocation"));

		if (bUseAttackLocation && Blackboard && Blackboard->IsVectorValueSet(AttackLocationKey))
		{
			AnchorLocationOverride = Blackboard->GetValueAsVector(AttackLocationKey);
		}

		Manager->RequestCameraPreset(TEXT("Attack.CloseUp"), Character, AnchorLocationOverride);
	}

	return EBTNodeResult::Succeeded;
}

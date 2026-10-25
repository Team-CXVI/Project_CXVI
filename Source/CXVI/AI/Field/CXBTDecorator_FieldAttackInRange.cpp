// Fill out your copyright notice in the Description page of Project Settings.


#include "CXBTDecorator_FieldAttackInRange.h"
#include "CXFieldAI.h"
#include "Interface/CXFieldEnemyAIInterface.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXBTDecorator_FieldAttackInRange)

UCXBTDecorator_FieldAttackInRange::UCXBTDecorator_FieldAttackInRange()
{
    bNotifyTick = true;

    NodeName = TEXT("Can Attack");
}

bool UCXBTDecorator_FieldAttackInRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    AAIController* Controller = OwnerComp.GetAIOwner();

    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

    if (!IsValid(Controller) || !Blackboard)
    {
        return false;
    }

    // 보이지 않는 타깃에게 새 공격을 시작하지 않음.
    if (!Blackboard->GetValueAsBool(BBEKY_HASTARGETSIGHT))
    {
        return false;
    }

    APawn* ControllingPawn = Controller->GetPawn();
    if (!IsValid(ControllingPawn))
    {
        return false;
    }

    ICXFieldEnemyAIInterface* AIPawn = Cast<ICXFieldEnemyAIInterface>(ControllingPawn);
    if (!AIPawn)
    {
        return false;
    }

    APawn* Target = Cast<APawn>(Blackboard->GetValueAsObject(BBKEY_TARGET));
    if (!IsValid(Target))
    {
        return false;
    }

    return ControllingPawn->GetDistanceTo(Target) <= AIPawn->GetAIAttackRange();
}

void UCXBTDecorator_FieldAttackInRange::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    ConditionalFlowAbort(OwnerComp, EBTDecoratorAbortRequest::ConditionResultChanged);
}

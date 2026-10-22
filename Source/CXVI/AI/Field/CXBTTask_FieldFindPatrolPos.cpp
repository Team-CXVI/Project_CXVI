// Fill out your copyright notice in the Description page of Project Settings.


#include "CXBTTask_FieldFindPatrolPos.h"
#include "CXFieldAI.h"
#include "Interface/CXFieldEnemyAIInterface.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "AIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXBTTask_FieldFindPatrolPos)

UCXBTTask_FieldFindPatrolPos::UCXBTTask_FieldFindPatrolPos()
{
    // 노드 이름 설정.
    NodeName = TEXT("FindPatrolPos");
}

EBTNodeResult::Type UCXBTTask_FieldFindPatrolPos::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    EBTNodeResult::Type Result = Super::ExecuteTask(OwnerComp, NodeMemory);

    // 다음에 정찰할 위치 선택 로직.


    // 네비게이션 시스템 활용을 위한 월드 객체 가져오기.
    APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
    if (!ControllingPawn)
    {
        return EBTNodeResult::Failed;
    }
    

    // 네비게이션 시스템을 활용하기 위한 포인터 얻기.
    UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetNavigationSystem(ControllingPawn->GetWorld());
    if (!NavSystem)
    {
        return EBTNodeResult::Failed;
    }

    // AI 인터페이스로 형변환.
    ICXFieldEnemyAIInterface* AIPawn = Cast<ICXFieldEnemyAIInterface>(ControllingPawn);

    // 폰이 생성된 초기 위치 가져오기.
    FVector Origin = OwnerComp.GetBlackboardComponent()->GetValueAsVector(BBKEY_HOMEPOS);

    // 정찰 반경 얻어오기.
    float PatrolRadius = AIPawn->GetAIPatrolRadius();

    // 네비게이션 기능 활용 - 이동 가능한 위치를 랜덤으로 선택.
    FNavLocation NextPatrolPos;
    if (NavSystem->GetRandomPointInNavigableRadius(Origin, PatrolRadius, NextPatrolPos))
    {
        // 선택된 랜덤 위치를 블랙보드에 기록.
        OwnerComp.GetBlackboardComponent()->SetValueAsVector(BBKEY_PATROLPOS, NextPatrolPos.Location);
        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::Failed;
}

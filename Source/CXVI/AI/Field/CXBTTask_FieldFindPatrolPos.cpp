// Fill out your copyright notice in the Description page of Project Settings.


#include "CXBTTask_FieldFindPatrolPos.h"
#include "CXFieldAI.h"
#include "Interface/CXFieldEnemyAIInterface.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
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
    if (!AIPawn)
    {
        return EBTNodeResult::Failed;
    }

    // 폰이 생성된 초기 위치 가져오기.
    const FVector PatrolCenter = AIPawn->GetAIPatrolCenter();

    // 정찰 반경 얻어오기.
    float PatrolRadius = AIPawn->GetAIPatrolRadius();

    if (PatrolRadius <= 0.0f)
    {
        return EBTNodeResult::Failed;
    }

    // 네비게이션 기능 활용 - 이동 가능한 위치를 랜덤으로 선택.
    // 한 번에 무한히 찾지 않도록 시도 횟수 제한.
    constexpr int32 MaxAttempts = 10;
    const float PatrolRadiusSquared = FMath::Square(PatrolRadius);

	for (int32 Attempt = 0;Attempt < MaxAttempts;++Attempt)
    {
        FNavLocation NextPatrolPos;

        if (!NavSystem->GetRandomPointInNavigableRadius(PatrolCenter, PatrolRadius, NextPatrolPos))
        {
            continue;
        }

        UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
            ControllingPawn->GetWorld(),
            ControllingPawn->GetNavAgentLocation(),
            NextPatrolPos.Location,
            ControllingPawn
        );

        // 목적지 까지 도달하는 완전한 경로만 허용.
        if (!Path || !Path->IsValid() || Path->IsPartial() || Path->PathPoints.Num() < 2)
        {
            UE_LOG(LogTemp, Display,
                TEXT("[Patrol] %s: 완전한 경로 없음. Goal=%s"),
                *ControllingPawn->GetName(),
                *NextPatrolPos.Location.ToString());

            continue;
        }

        bool bInsidePatrolArea = true;

        for (const FVector& Point : Path->PathPoints)
        {
            if (FVector::DistSquared(Point, PatrolCenter) > PatrolRadiusSquared)
            {
                UE_LOG(LogTemp, Display,
                    TEXT("[Patrol] %s: 구역 이탈 경로 거절. Goal=%s OutsidePoint=%s Distance=%.1f Radius=%.1f"),
                    *ControllingPawn->GetName(),
                    *NextPatrolPos.Location.ToString(),
                    *Point.ToString(),
                    FVector::Dist(Point, PatrolCenter),
                    PatrolRadius);

                bInsidePatrolArea = false;
                break;
            }

        }
        
        // 경로가 담당 구역을 벗어나면 다음 후보를 검사.
        if (!bInsidePatrolArea)
        {
            continue;
        }

        UE_LOG(LogTemp, Display,
            TEXT("[Patrol] %s: 후보 경로 통과. Goal=%s"),
            *ControllingPawn->GetName(),
            *NextPatrolPos.Location.ToString());

        OwnerComp.GetBlackboardComponent()->SetValueAsVector(BBKEY_PATROLPOS, NextPatrolPos.Location);

        return EBTNodeResult::Succeeded;
    }

    // 유효한 후보가 없으면 이전 목적지를 남기지 않음,
    OwnerComp.GetBlackboardComponent()->ClearValue(BBKEY_PATROLPOS);

    return EBTNodeResult::Failed;
}

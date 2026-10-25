// Fill out your copyright notice in the Description page of Project Settings.


#include "CXBTTask_FieldPatrolMove.h"
#include "CXFieldAIController.h"

#include "AITypes.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXBTTask_FieldPatrolMove)

namespace
{
	struct FFieldPatrolMoveMemory
	{
		FAIRequestID MoveRequestID;
	};
}

UCXBTTask_FieldPatrolMove::UCXBTTask_FieldPatrolMove()
{
	NodeName = TEXT("Field Patrol Move");

	// 이동 목적지는 Vector 타입의 블랙보드 키만 허용.
	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UCXBTTask_FieldPatrolMove, BlackboardKey));
}

EBTNodeResult::Type UCXBTTask_FieldPatrolMove::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	auto* Memory = reinterpret_cast<FFieldPatrolMoveMemory*>(NodeMemory);
	Memory->MoveRequestID = FAIRequestID::InvalidRequest;

	ACXFieldAIController* Controller = Cast<ACXFieldAIController>(OwnerComp.GetAIOwner());

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

	if (!Controller || !Controller->GetPawn() || !Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	const FName GoalKey = GetSelectedBlackboardKey();
	if (!Blackboard->IsVectorValueSet(GoalKey))
	{
		return EBTNodeResult::Failed;
	}

	// 블랙보드의 정찰 목적지로 이동 요청 구성.
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(Blackboard->GetValueAsVector(GoalKey));
	MoveRequest.SetUsePathfinding(true);
	MoveRequest.SetAllowPartialPath(false);
	MoveRequest.SetAcceptanceRadius(FMath::Max(0.0f, AcceptanceRadius));
	MoveRequest.SetReachTestIncludesAgentRadius(false);
	MoveRequest.SetReachTestIncludesGoalRadius(false);

	UPathFollowingComponent* PathFollowing = Controller->GetPathFollowingComponent();

	if (!PathFollowing)
	{
		return EBTNodeResult::Failed;
	}

	// 이미 목적지에 도착해 있다면 이동 없이 종료.
	if (PathFollowing->HasReached(MoveRequest))
	{
		return EBTNodeResult::Succeeded;
	}

	// 이 Enemy의 네비게이션 설정으로 실제 이동 경로 계산.
	FPathFindingQuery Query;

	if (!Controller->BuildPathfindingQuery(MoveRequest, Query))
	{
		return EBTNodeResult::Failed;
	}

	FNavPathSharedPtr Path;
	Controller->FindPathForMoveRequest(MoveRequest, Query, Path);

	// 실제로 사용할 경로가 담당 구역 안인지 확인.
	if (!Controller->IsPathInsidePatrolArea(Path))
	{
		return EBTNodeResult::Failed;
	}

	// 검사를 거치지 않은 경로로 자동 변경되지 않도록 설정.
	// 이동 중 경로가 무효화 되면 실패하고 다음 정찰 시 다시 계산.
	Path->EnableRecalculationOnInvalidation(false);

	// 새로 계산하지 않고, 검사한 경로르 그대로 전달.
	Memory->MoveRequestID = Controller->RequestMove(MoveRequest, Path);
	if (!Memory->MoveRequestID.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	// 이 이동 요청의 완료 메시지를 기다림.
	WaitForMessage(OwnerComp, UBrainComponent::AIMessage_MoveFinished, Memory->MoveRequestID);


	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UCXBTTask_FieldPatrolMove::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	auto* Memory = reinterpret_cast<FFieldPatrolMoveMemory*>(NodeMemory);

	// 취소하면서 발생하는 완료 메시지를 더 이상 받지 않음.
	StopWaitingForMessages(OwnerComp);

	ACXFieldAIController* Controller = Cast<ACXFieldAIController>(OwnerComp.GetAIOwner());
	if (Controller && Memory->MoveRequestID.IsValid())
	{
		UPathFollowingComponent* PathFollowing = Controller->GetPathFollowingComponent();
		if (PathFollowing)
		{
			// 이 Task가 시작한 정찰 이동만 취소.
			PathFollowing->AbortMove(*this, FPathFollowingResultFlags::OwnerFinished, Memory->MoveRequestID);
		}

	}
	

	Memory->MoveRequestID = FAIRequestID::InvalidRequest;

	return EBTNodeResult::Aborted;
}

uint16 UCXBTTask_FieldPatrolMove::GetInstanceMemorySize() const
{
	return sizeof(FFieldPatrolMoveMemory);
}
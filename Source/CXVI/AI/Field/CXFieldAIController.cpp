// Fill out your copyright notice in the Description page of Project Settings.


#include "CXFieldAIController.h"
#include "CXFieldAI.h"
#include "Interface/CXFieldEnemyAIInterface.h"

#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"


ACXFieldAIController::ACXFieldAIController()
{
	BBAsset = CreateDefaultSubobject<UBlackboardData>(TEXT("BlackboardData"));

	BTAsset = CreateDefaultSubobject<UBehaviorTree>(TEXT("BehaviorTree"));
}

void ACXFieldAIController::RunAI()
{
	ICXFieldEnemyAIInterface* AIPawn = Cast<ICXFieldEnemyAIInterface>(GetPawn());
	if (!AIPawn)
	{
		return;
	}

	// 블랙보드 컴포넌트 가져오기.
	UBlackboardComponent* BlackboardPtr = Blackboard;

	// 사용할 블랙보드 지정.
	if (UseBlackboard(BBAsset, BlackboardPtr))
	{
		// Enemy 폰이 생성된 위치를 블랙보드에 기록.
		BlackboardPtr->SetValueAsVector(BBKEY_HOMEPOS, AIPawn->GetAIPatrolCenter());

		// 비헤이비어 트리 실행.
		bool RunResult = RunBehaviorTree(BTAsset);

		// 실행 성공여부 확인.
		ensureAlways(RunResult);
	}
}

void ACXFieldAIController::StopAI()
{
	// 실행 중인 behaviorTreeComponent 가져오기.
	UBehaviorTreeComponent* BTComponent = Cast<UBehaviorTreeComponent>(BrainComponent);
	if (BTComponent)
	{
		// BT 실행 중지.
		BTComponent->StopTree();
	}
}

void ACXFieldAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// AI 실행.
	RunAI();
}

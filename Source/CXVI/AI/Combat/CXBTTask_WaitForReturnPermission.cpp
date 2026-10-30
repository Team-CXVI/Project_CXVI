// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Combat/CXBTTask_WaitForReturnPermission.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXBTTask_WaitForReturnPermission)

UCXBTTask_WaitForReturnPermission::UCXBTTask_WaitForReturnPermission()
{
	NodeName = TEXT("Wait For Return");

	bNotifyTick = true;
}

EBTNodeResult::Type UCXBTTask_WaitForReturnPermission::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	if (!BB || BB->GetKeyID(TEXT("CanReturn")) == FBlackboard::InvalidKey)
	{
		UE_LOG(LogTemp, Warning, TEXT("복귀 대기 실패: 블랙보드 또는 CanReturn 키값이 없습니다."));
		return EBTNodeResult::Failed;
	}

	// 태스크 진입 전에 허가가 도착했어도 정상적으로 진행한다.
	if (BB->GetValueAsBool(TEXT("CanReturn")))
	{
		UE_LOG(LogTemp, Display, TEXT("복귀 허가 확인: 복귀 시작"));
		return EBTNodeResult::Succeeded;
	}

	UE_LOG(LogTemp, Display, TEXT("공격 종료: 복귀 허가 대기"));
	return EBTNodeResult::InProgress;
}

void UCXBTTask_WaitForReturnPermission::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	if (!BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (BB->GetValueAsBool(TEXT("CanReturn")))
	{
		UE_LOG(LogTemp, Display, TEXT("복귀 허가 수신: 복귀 시작"));
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

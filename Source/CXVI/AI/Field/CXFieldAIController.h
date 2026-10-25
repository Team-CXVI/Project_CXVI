// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CXFieldAIController.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API ACXFieldAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACXFieldAIController();
	
	// AI 실행 함수.
	void RunAI();

	// AI 중지 함수.
	void StopAI();

	// 완전한 경로이며 모든 경로 점이 담당 구역 안인지 확인.
	bool IsPathInsidePatrolArea(const FNavPathSharedPtr& Path) const;

protected:
	// 컨트롤러가 폰에 빙의할 때 호출되는 함수.
	virtual void OnPossess(APawn* InPawn)override;

protected:
	// 블랙보드 에셋.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "AI")
	TObjectPtr<class UBlackboardData > BBAsset;

	// 비헤이비어 트리 에셋.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TObjectPtr<class UBehaviorTree> BTAsset;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_FieldDetect.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UBTService_FieldDetect : public UBTService
{
	GENERATED_BODY()
public:
	UBTService_FieldDetect();

protected:
	// 서비스가 실행될 때마다 호출되는 함수 오버라이드.
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	
};

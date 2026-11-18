// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "CXBTTask_DashToLocation.generated.h"

class ACharacter;
class UAnimInstance;
class UAnimMontage;
class UMotionWarpingComponent;

/**
 * 
 */
UCLASS()
class CXVI_API UCXBTTask_DashToLocation : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UCXBTTask_DashToLocation();

protected:
    EBTNodeResult::Type ExecuteTask(
        UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    EBTNodeResult::Type AbortTask(
        UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
    void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted, uint32 DashId);
    void ClearDashState();

private:


    UPROPERTY(EditAnywhere, Category = "Dash")
    FBlackboardKeySelector DestinationKey;

    UPROPERTY(VisibleAnywhere, Category = "Dash")
    TObjectPtr<UAnimMontage> DashMontage;

    UPROPERTY(EditAnywhere, Category = "Dash", meta = (ClampMin = "0.0"))
    float ArrivalTolerance = 40.f;

    TWeakObjectPtr<UBehaviorTreeComponent> ActiveTree;
    TWeakObjectPtr<ACharacter> DashCharacter;
    TWeakObjectPtr<UAnimInstance> DashAnimInstance;
    TWeakObjectPtr<UMotionWarpingComponent> MotionWarping;

    FVector Destination = FVector::ZeroVector;
    uint32 ActiveDashId = 0;
};

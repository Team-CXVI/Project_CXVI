// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CXCombatAIController.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API ACXCombatAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	ACXCombatAIController();

	bool RequestCombatAction(
		AActor* Target,
		const FVector& AttackLocation,
		const FVector& OriginLocation,
		bool bShouldApproach);

	bool EndCombatAction();

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Combat|AI")
	TObjectPtr<class UBehaviorTree> CombatBehaviorTree;
};

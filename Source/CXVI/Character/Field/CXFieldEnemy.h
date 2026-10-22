// Fill out your copyright notice in the Description page of Project Settings.

#pragma once



#include "CoreMinimal.h"
#include "CXVI/AI/Field/Interface/CXFieldEnemyAIInterface.h"

#include "GameFramework/Character.h"
#include "CXFieldEnemy.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API ACXFieldEnemy : public ACharacter, public ICXFieldEnemyAIInterface
{
	GENERATED_BODY()

public:
	ACXFieldEnemy();

protected:
	virtual void BeginPlay() override;


protected:
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="PreemptiveAttack")
	TObjectPtr<class UChildActorComponent> PreemptiveAttackTrigger;



	// AI 섹션.
protected:
	float GetAIPatrolRadius() override;
	float GetAIDetectRange() override;
	float GetAIAttackRange() override;
	float GetAITurnSpeed() override;

};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CXFieldEnemyAIInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCXFieldEnemyAIInterface : public UInterface
{
	GENERATED_BODY()
};

// 공격 종료 시에 호출할 델리게이트 선언.
DECLARE_DELEGATE(FAICharacterAttackFinished);

/**
 * 
 */
class CXVI_API ICXFieldEnemyAIInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// 에너미가 제공할 함수.
	virtual float GetAIPatrolRadius() = 0;
	virtual FVector GetAIPatrolCenter() = 0;
	virtual float GetAIDetectRange() = 0;
	virtual float GetAIAttackRange() = 0;
	virtual float GetAITurnSpeed() = 0;
	virtual float GetAIChaseRadius() = 0;
	virtual float GetAISightHalfAngle() = 0;

	// AI를 통해 공격 메시지를 전달할 떄 사용할 함수.
	virtual bool AttackByAI() = 0;
	virtual void CancelAttackByAI() = 0;
	virtual bool IsAIAttacking() const = 0;

	// 캐릭터에 델리게이트를 전달할 때 사용할 함수.
	virtual void SetAIAttackDelegate(const FAICharacterAttackFinished& InOnAttackFinished) = 0;

	// 공격 종료 시에 호출할 델리게이트
	FAICharacterAttackFinished OnAttackFinished;
	
	
	
};

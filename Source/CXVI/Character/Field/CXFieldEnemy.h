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

public:
	// Getter.
	FORCEINLINE float GetAttackRadius() const { return AttackRadius; }

	// 생성 초기화 과정에서 스포너가 호출.
	void InitializePatrolRadius(float InPatrolRadius);

protected:
	virtual void BeginPlay() override;


protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TSubclassOf<class AAIController> EnemyAIController;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="PreemptiveAttack")
	TObjectPtr<class UChildActorComponent> PreemptiveAttackTrigger;

	// 공격 반경.
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Attack")
	float AttackRadius;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "AI|Patrol")
	float PatrolRadius = 500.0f;

protected:
	void AttackEnded(UAnimMontage* Montage, bool bInterrupted);

	// 공격이 끝나느 시점에 호출할 함수.
	virtual void NotifyActionEnd();

	// AI 섹션.
protected:
	float GetAIPatrolRadius() override;
	float GetAIDetectRange() override;
	float GetAIAttackRange() override;
	float GetAITurnSpeed() override;
	virtual void AttackByAI() override;
	virtual void SetAIAttackDelegate(const FAICharacterAttackFinished& InOnAttackFinished) override;

};

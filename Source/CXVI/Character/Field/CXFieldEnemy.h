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

	// AI 섹션.
public:
	// 생성 초기화 과정에서 스포너가 호출.
	// 생성 완료 전에 스포너가 담당 구역 정보를 전달.
	void InitializePatrolArea(const FVector& InPatrolCenter,float InPatrolRadius);

	// 스포너 섹션.
public:
	// 생성 완료 전에 스포너가 호출.
	void InitializeSpawnId(FName InSpawnId);

	UFUNCTION(Blueprintpure, Category = "Field|Enemy")
	FName GetSpawnId() const { return SpawnId; }

	// 처치 확정 시 호출. 기록에 성공하면 액터를 제거한다.
	UFUNCTION(BlueprintCallable, Category = "Field|Enemy")
	bool Defeat();

protected:
	virtual void BeginPlay() override;

protected:
	void AttackEnded(UAnimMontage* Montage, bool bInterrupted);

	// 공격이 끝나는 시점에 호출할 함수.
	virtual void NotifyActionEnd();

	// AI Interface섹션.
protected:
	virtual float GetAIPatrolRadius() override;
	virtual FVector GetAIPatrolCenter() override;
	virtual float GetAIDetectRange() override;
	virtual float GetAIAttackRange()  override;
	virtual float GetAITurnSpeed() override;
	virtual float GetAIChaseRadius() override;
	virtual bool AttackByAI() override;
	virtual void CancelAttackByAI() override;
	virtual bool IsAIAttacking() const override;
	virtual void SetAIAttackDelegate(const FAICharacterAttackFinished& InOnAttackFinished) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TSubclassOf<class AAIController> EnemyAIController;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	TObjectPtr<UAnimMontage> AttackMontage;

	// AI 공격이 이동을 제한한 상태인지 구분.
	UPROPERTY(Transient)
	bool bAIAttacking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PreemptiveAttack")
	TObjectPtr<class UChildActorComponent> PreemptiveAttackTrigger;

	// 공격 반경.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float AttackRadius;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "AI|Patrol")
	float PatrolRadius = 500.0f;

	// 담당 구역의 고정된 월드 좌표
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "AI|Patrol")
	FVector PatrolCenter = FVector::ZeroVector;

	// 정찰 구역 밖으로 추가 추적을 허용할 거리.
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="AI|Chase")
	float ExtraChaseDistance = 400.0f;

	// 스포너가 담당 구역을 지정했는지 구분.
	UPROPERTY(Transient)
	bool bPatrolAreaInitialize = false;


	// 스포너 변수.
	// 이 몬스터가 어느 스폰 지점에서 생성됐는지 식별.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Field|Enemy")
	FName SpawnId = NAME_None;

	// 처치 처리를 중복 실행하지 않도록 구분.
	UPROPERTY(Transient)
	bool bDefeatHandled = false;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once



#include "CoreMinimal.h"
#include "CXVI/AI/Field/Interface/CXFieldEnemyAIInterface.h"
#include "CXVI/Foundation/Scene/CXSceneSuspendable.h"

#include "GameFramework/Character.h"
#include "CXFieldEnemy.generated.h"

class USoundBase;
class ACXFieldPlayerCharacter;

/**
 * 
 */
UCLASS()
class CXVI_API ACXFieldEnemy : public ACharacter, public ICXFieldEnemyAIInterface, public ICXSceneSuspendable
{
	GENERATED_BODY()

public:
	ACXFieldEnemy();

	virtual void Tick(float DeltaSeconds) override;

public:
	// Getter.
	FORCEINLINE float GetAttackRadius() const { return AttackRadius; }

	// AI 섹션.
public:
	// 생성 초기화 과정에서 스포너가 호출.
	// 생성 완료 전에 스포너가 담당 구역 정보를 전달.
	void InitializePatrolArea(const FVector& InPatrolCenter,float InPatrolRadius);

	// AI를 Duration 동안 멈춘 다음 정찰 시작.
	void PauseAIForSeconds(float Duration);

	// 스포너 섹션.
public:
	// 생성 완료 전에 스포너가 호출.
	void InitializeSpawnId(FName InSpawnId);

	UFUNCTION(Blueprintpure, Category = "Field|Enemy")
	FName GetSpawnId() const { return SpawnId; }

	// 처치 확정 시 호출. 기록에 성공하면 액터를 제거한다.
	UFUNCTION(BlueprintCallable, Category = "Field|Enemy")
	bool Defeat();

	// 에너미와 상호작용.
public:
	// 생성 완료 전에 스포너가 전달.
	void InitializeEncounterId(FName InEncounterId);

	FName GetEncounterId() const { return EncounterId; }

	// 공격 몽타주의 타격 Notify에서 호출.
	void NotifyAttackHit();

	bool CanBeAttacked() const { return !bPlayerCollisionIgnored; }

	// 필드 씬 인터페이스 섹션.
public:
	void OnSceneSuspended_Implementation();

	void OnSceneResumed_Implementation();

	// 연출.
protected:
	// 공격 명중 시 슬로우 + 카메라 당기기 연출 후 전투 진입 요청.
	void SuccessedAttackHit(ACXFieldPlayerCharacter* Player);

	// 필드 몬스터 전체 AI 정지(true) / 재개(false).
	void SetFieldEnemiesSuspended(UWorld* World, bool bSuspended) const;

	// 몬스터 선공으로 전투 진입 요청. 반환값: 요청 성공 여부.
	bool RequestEnemyAttackBattle(UWorld* World);

protected:
	virtual void BeginPlay() override;

protected:
	void AttackEnded(UAnimMontage* Montage, bool bInterrupted);

	// 공격이 끝나는 시점에 호출할 함수.
	virtual void NotifyActionEnd();

	bool FaceAttackTarget();

	// 회전까지 끝난 뒤 이동 복구와 공격 온료 알림 처리.
	void FinishAttackAction();

	void ResumeAI();

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
	virtual float GetAISightHalfAngle() override;
	virtual float GetAISightLossGraceTime() override;
	virtual void OnAITargetDetected() override;

	// 사운드 섹션.
protected:
	// 타깃이 없을 때 Idle 사운드 재생 후 다음 재생 예약.
	void PlayIdleSound();

protected:
	// 평소에 가끔 내는 소리.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> IdleSound = nullptr;

	// Idle 사운드 재생 간격(초). 이 범위 안에서 랜덤.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	float IdleSoundMinInterval = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	float IdleSoundMaxInterval = 14.0f;

	// 플레이어를 발견했을 때 내는 소리.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> DetectSound = nullptr;

	FTimerHandle IdleSoundTimerHandle;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TSubclassOf<class AAIController> EnemyAIController;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	TObjectPtr<UAnimMontage> AttackMontage;

	// AI 공격이 이동을 제한한 상태인지 구분.
	UPROPERTY(Transient)
	bool bAIAttacking = false;

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

	// 정면 기준 한쪽 시야각. 60이면 전체 시야각은 120도.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sight")
	float SightHalfAngle = 60.0f;

	// 플레이어를 감지하는 거리.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sight")
	float DetectRange = 400.0f;

	// 타깃이 보이지 않아도 추적을 유지할 시간.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="AI|Sight")
	float SightLossGraceTime = 2.0f;

	// 공격 종료 후 방향 정렬 중인지 구분.
	UPROPERTY(Transient)
	bool bTurningAfterAttack = false;

	// 이번 방향 전환의 목표 각도.
	float AttackTargetYaw = 0.0f;

	// 초당 회전 각도. 540이면 180도 회전에 약 0.33초.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Attack")
	float AttackTurnSpeed = 540.0f;

	// 실제 타격 판정의 전방 도달 거리.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Hit")
	float AttackHitRange = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Hit")
	float AttackHitRadius = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Hit")
	bool bDrawAttackHitDebug = false;


	// 스포너 변수.
	// 이 몬스터가 어느 스폰 지점에서 생성됐는지 식별.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Field|Enemy")
	FName SpawnId = NAME_None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Field|Enemy")
	FName EncounterId = NAME_None;

	// 처치 처리를 중복 실행하지 않도록 구분.
	UPROPERTY(Transient)
	bool bDefeatHandled = false;

	FTimerHandle AIPauseTimerHandle;

	UPROPERTY(Transient)
	bool bPlayerCollisionIgnored = false;
};

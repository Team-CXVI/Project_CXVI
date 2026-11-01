// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ActiveGameplayEffectHandle.h"
#include "Engine/TimerHandle.h"
#include "CXPlayerGrapplingHookAbility.generated.h"

class AActor;
class ACXFieldPlayerCharacter;
class UAnimMontage;

UENUM(BlueprintType)
enum class ECXGrapplingPhase :uint8
{
	None,
	Preparing,
	Flying,
	Landing
};

/**
 * 
 */
UCLASS()
class CXVI_API UCXPlayerGrapplingHookAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UCXPlayerGrapplingHookAbility();
	
protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 플레이어가 착지 헀는지 알기위한 함수.
	UFUNCTION()
	void HandleGrapplingLanded(const FHitResult& Hit);

protected:
	// 애니메이션.
	UFUNCTION()
	void OnGrapplingInterrupted();

	UFUNCTION()
	void OnGrapplingMontageCompleted();

protected:
	UFUNCTION()
	void OnGrapplingMoveReached();

	void StartGrapplingMontage();

	// 착지 전에 Flight Section이 끝나면 마지막 자세로 멈춰서 Land Section으로 넘어가지 않게 함.
	void HoldFlightPoseUntilLanded();

	void ResumeGrapplingMontage();

protected:
	// 세부 애니메이션
	UFUNCTION()
	void OnShowCableEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnStartMoveEvent(FGameplayEventData Payload);

	UPROPERTY(Transient)
	ECXGrapplingPhase GrapplingPhase = ECXGrapplingPhase::None;

	UPROPERTY(EditDefaultsOnly, Category="Grappling",meta=(ClampMin="0.1"))
	float GrapplingTimeOut = 8.0f;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grappling")
	float GrapplingVelocity = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grappling")
	float FlyingSpeed = 800.0f;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> GrapplingTarget;

	UPROPERTY(Transient)
	TWeakObjectPtr<ACXFieldPlayerCharacter> GrapplingCharacter;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> LandingSurface;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grappling")
	TObjectPtr<UAnimMontage> GrapplingMontage;

	// 이동 경로의 높이 등을 설정한 커브.
	UPROPERTY(EditDefaultsOnly, Category = "Grappling")
	TObjectPtr<class UCurveVector> GrapplingPathCurve;

	// 실행 중인 이동 Task.
	UPROPERTY(Transient)
	TObjectPtr<class UAbilityTask_ApplyRootMotionMoveToForce> GrapplingMoveTask;

	// 목적지 까지 이동할 시간.
	UPROPERTY(EditDefaultsOnly, Category = "Grappling",meta=(ClampMin="0.05"))
	float GrapplingDuration = 0.8f;

	// 이번 실행에서 확정할 목적지.
	FVector GrapplingGoal = FVector::ZeroVector;
	
	// 이번 어빌리티가 이동 모드를 변경했는지.
	bool bControlsGrapplingMovement = false;

	FTimerHandle GrapplingAirHoldTimerHandle;

	// 착지 대기로 몽타주를 멈췄는지.
	bool bGrapplingMontagePaused = false;

};

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
class UNiagaraSystem;
class UNiagaraComponent;
class UCameraComponent;
class USoundBase;
class UAudioComponent;

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

	void StartFOVBlend(float TargetFOV, float Duration);
	void TickFOVBlend();

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
	
	// 나이아가라 이펙트.
	UPROPERTY(EditDefaultsOnly, Category = "FX")
	TObjectPtr<UNiagaraSystem> HandFX;

	UPROPERTY(EditDefaultsOnly, Category = "FX") 
	TObjectPtr<UNiagaraSystem> BodyFX;

	UPROPERTY(EditDefaultsOnly, Category = "FX") 
	TObjectPtr<UNiagaraSystem> LandDustFX;

	UPROPERTY(EditDefaultsOnly, Category = "FX") 
	FName HandSocket = TEXT("hand_r");

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> HandFXComp;

	// 사운드.
	// 케이블이 나갈 때.
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> CableSound;

	// 날아가는 동안. 착지하거나 어빌리티가 끝나면 멈춤.
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> FlightSound;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> FlightAudioComp;

	void StopFlightSound();

	// 비행 FOV
	UPROPERTY(EditDefaultsOnly, Category = "Grappling|Camera")
	float FlightFOVAdd = 15.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Grappling|Camera")
	float FOVBlendInTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Grappling|Camera")
	float FOVBlendOutTime = 0.3f;

	TWeakObjectPtr<UCameraComponent> GrapplingCamera;
	float BaseFOV = 90.0f;
	float FOVFrom = 90.0f;
	float FOVTo = 90.0f;
	float FOVBlendDuration = 0.0f;
	float FOVBlendStartTime = 0.0f;
	bool bFOVChanged = false;
	FTimerHandle FOVBlendTimerHandle;

	// 카메라 거리 (SpringArm TargetArmLength)
	// 팔 뻗을 때(ShowCable) 멀어짐 -> 날아갈 때 가까이 붙음 -> 비행 중 원래 거리로 복귀.
	UPROPERTY(EditDefaultsOnly, Category = "Grappling|Camera")
	float ArmPushOutDistance = 150.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Grappling|Camera", meta = (ClampMin = "0.01"))
	float ArmPushOutTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Grappling|Camera")
	float ArmPullInDistance = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Grappling|Camera", meta = (ClampMin = "0.01"))
	float ArmPullInTime = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Grappling|Camera", meta = (ClampMin = "0.01"))
	float ArmReturnTime = 0.4f;

	// 가까이 붙을 때 최소 거리.
	UPROPERTY(EditDefaultsOnly, Category = "Grappling|Camera", meta = (ClampMin = "0.0"))
	float MinArmLength = 80.0f;

	TWeakObjectPtr<class USpringArmComponent> GrapplingCameraBoom;
	float BaseArmLength = 300.0f;
	float ArmFrom = 300.0f;
	float ArmTo = 300.0f;
	float ArmBlendDuration = 0.0f;
	float ArmBlendStartTime = 0.0f;
	bool bArmChanged = false;

	// 현재 보간이 끝나면 이어서 갈 목표. (음수면 없음)
	float ArmNextTarget = -1.0f;
	float ArmNextDuration = 0.0f;
	FTimerHandle ArmBlendTimerHandle;

	void StartArmBlend(float TargetLength, float Duration, float NextTarget = -1.0f, float NextDuration = 0.0f);
	void TickArmBlend();
	void RestoreArmLength();

	// 몸 방향.
	// 사선으로 날아갈 때 몸 정면과 이동 방향이 어긋나 비틀어지지 않도록,
	// 준비 단계에서 착지 방향으로 Yaw만 돌리고 비행 중에는 회전을 고정합니다.
	UPROPERTY(EditDefaultsOnly, Category = "Grappling|Rotation", meta = (ClampMin = "0.01"))
	float FaceGoalTime = 0.15f;

	FQuat FaceFromRot = FQuat::Identity;
	FQuat FaceToRot = FQuat::Identity;
	float FaceStartTime = 0.0f;
	FTimerHandle FaceGoalTimerHandle;

	bool bRotationLocked = false;
	bool bSavedOrientRotationToMovement = false;
	bool bSavedUseControllerDesiredRotation = false;

	void StartFaceGoal();
	void TickFaceGoal();
	void FinishFaceGoal();
	void LockFlightRotation();
	void RestoreFlightRotation();


};

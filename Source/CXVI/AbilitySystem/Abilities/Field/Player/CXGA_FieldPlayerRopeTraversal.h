// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CXVI/Actor/Field/Interactions/CXRopeTraversalActor.h"
#include "Abilities/GameplayAbility.h"
#include "CXGA_FieldPlayerRopeTraversal.generated.h"

/**
 * 
 */
class ACXFieldPlayerCharacter;
class UAbilityTask_ApplyRootMotionMoveToForce;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitDelay;
class UAnimMontage;
class UCameraComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class USoundBase;
class UAudioComponent;

USTRUCT(BlueprintType)
struct FCXRopeTraversalMontageSet
{
	GENERATED_BODY()

	// 로프에 붙을 떄.
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> AttachMontage;

	// 로프에서 이동중.
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> TraversalMontage;
	
	// 로프에 착지.
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> ExitMontage;
};

UCLASS()
class CXVI_API UCXGA_FieldPlayerRopeTraversal : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UCXGA_FieldPlayerRopeTraversal();

protected:
    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	// 플레이어 로프 이동.
protected:

	UFUNCTION()
	void OnTraversalInterrupted();

	// 이동 경로.
	void StartMoveToDestination();

	UFUNCTION()
	void OnDestinationFinished();

	UPROPERTY(EditDefaultsOnly, Category = "Rope")
	float TraversalSpeed = 600.0f;

	UPROPERTY(Transient)
	FVector DestinationMoveGoal = FVector::ZeroVector;

	// 탈출점.
	void StartMoveToExit();

	UFUNCTION()
	void OnExitMoveFinished();

	UPROPERTY(EditDefaultsOnly, Category = "Rope")
	float ExitMoveDuration = 0.3f;

	UPROPERTY(Transient)
	FVector ExitMoveGoal = FVector::ZeroVector;

	// 착지.
	UFUNCTION()
	void HandleTraversalLanded(const FHitResult& Hit);

	bool bWaitingForLanding = false;

	UPROPERTY(EditDefaultsOnly, Category = "Rope")
	float LandingTimeOut = 3.0f;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> LandingTimeoutTask;

	// 착지 몽타주가 끝날 때까지 어빌리티를 유지합니다.
	UFUNCTION()
	void OnExitMontageFinished();

	void TryFinishExit();

	bool bExitLanded = false;
	bool bExitMontageFinished = false;

	// 서 있던 자리에서 시작 몽타주를 재생하며 바로 로프 진입점에 붙는 이동.
	void StartMoveToRopeAttachment();

	UFUNCTION()
	void OnRopeAttachmentFinished();

	UPROPERTY(Transient)
	FVector AttachmentMoveGoal = FVector::ZeroVector;

	// 로프에 붙는 데 걸리는 시간. 시작 몽타주에서 손이 로프를 잡는 시점에 맞춥니다.
	UPROPERTY(EditDefaultsOnly, Category = "Rope|Attach", meta = (ClampMin = "0.05"))
	float AttachDuration = 0.7f;

	// 붙는 경로 오프셋 커브. (X: 0~1 정규화 시간, 값: cm 오프셋) 비우면 직선.
	UPROPERTY(EditDefaultsOnly, Category = "Rope|Attach")
	TObjectPtr<class UCurveVector> AttachCurvePath;

	// 붙는 동안 로프 진행 방향으로 몸을 돌리는 시간.
	UPROPERTY(EditDefaultsOnly, Category = "Rope|Attach", meta = (ClampMin = "0.01"))
	float FaceRopeTime = 0.25f;

	FQuat FaceFromRot = FQuat::Identity;
	FQuat FaceToRot = FQuat::Identity;
	float FaceStartTime = 0.0f;
	FTimerHandle FaceRopeTimerHandle;

	void StartFaceRope(const FVector& FaceDirection);
	void TickFaceRope();
	void StopFaceRope();

	const FCXRopeTraversalMontageSet& GetMontageSet() const
	{
		return TraversalDirection == ECXRopeTraversalDirection::Up ? UpMontages : DownMontages;
	}
	
	UAbilityTask_PlayMontageAndWait* PlayTraversalMontage(UAnimMontage* Montage, FName TaskName, bool bStopWhenAbilityEnds = true);

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<ACXFieldPlayerCharacter> TraversalCharacter;

	UPROPERTY(Transient)
	TWeakObjectPtr<ACXRopeTraversalActor> TraversalRope;

	UPROPERTY(Transient)
	ECXRopeTraversalDirection TraversalDirection = ECXRopeTraversalDirection::None;

	UPROPERTY(Transient)
	FVector TraversalEntry = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector TraversalDestination = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector TraversalExit = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_ApplyRootMotionMoveToForce> TraversalMoveTask;

	bool bControlsTraversalMovement = false;

	UPROPERTY(EditDefaultsOnly, Category = "Rope")
	TObjectPtr<class UCurveVector> ExitUpCurvePath;

	UPROPERTY(EditDefaultsOnly, Category = "Rope")
	TObjectPtr<class UCurveVector> ExitDownCurvePath;

	// 상승.
	UPROPERTY(EditDefaultsOnly, Category = "Rope|Montage")
	FCXRopeTraversalMontageSet UpMontages;

	// 하강.
	UPROPERTY(EditDefaultsOnly, Category = "Rope|Montage")
	FCXRopeTraversalMontageSet DownMontages;

	// 나이아가라 이펙트.
	// 진입~도착: 오른손 / 착지: 먼지.
protected:
	UPROPERTY(EditDefaultsOnly, Category = "FX")
	TObjectPtr<UNiagaraSystem> HandFX;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	TObjectPtr<UNiagaraSystem> LandDustFX;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	FName HandSocket = TEXT("FX_Hand_r");

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> HandFXComp;

	// 사운드.
	// 로프를 타고 이동하는 동안. 도착하거나 어빌리티가 끝나면 멈춤.
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> RideSound;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> RideAudioComp;

	void StopRideSound();

	// 로프를 타는 동안 FOV 넓힘. (도착하면 원래대로)
	UPROPERTY(EditDefaultsOnly, Category = "Rope|Camera")
	float RideFOVAdd = 12.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rope|Camera", meta = (ClampMin = "0.01"))
	float FOVBlendInTime = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Rope|Camera", meta = (ClampMin = "0.01"))
	float FOVBlendOutTime = 0.3f;

	TWeakObjectPtr<UCameraComponent> TraversalCamera;
	float BaseFOV = 90.0f;
	float FOVFrom = 90.0f;
	float FOVTo = 90.0f;
	float FOVBlendDuration = 0.0f;
	float FOVBlendStartTime = 0.0f;
	bool bFOVChanged = false;
	FTimerHandle FOVBlendTimerHandle;

	void StartFOVBlend(float TargetFOV, float Duration);
	void TickFOVBlend();
	void RestoreFOV();

};

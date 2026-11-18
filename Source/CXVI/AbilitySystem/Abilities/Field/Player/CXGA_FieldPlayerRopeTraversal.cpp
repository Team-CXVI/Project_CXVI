// Fill out your copyright notice in the Description page of Project Settings.


#include "CXGA_FieldPlayerRopeTraversal.h"
#include "CXVI/Character/Field/CXFieldPlayerCharacter.h"
#include "CXVI/Actor/Field/Interactions/CXRopeTraversalActor.h"
#include "CXVI/Tags/CXFieldTags.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Curves/CurveVector.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGA_FieldPlayerRopeTraversal)

namespace
{
	// bImmediate=false : 스폰만 멈추고 남은 파티클은 수명대로 사라짐. (AutoDestroy로 스폰해서 끝나면 자동 제거)
	// bImmediate=true  : 남은 파티클까지 즉시 제거. 수명이 무한/루프인 이펙트는 이걸 써야 꺼짐.
	void DeactivateRopeFX(TObjectPtr<UNiagaraComponent>& FXComp, bool bImmediate = false)
	{
		if (IsValid(FXComp))
		{
			if (bImmediate)
			{
				FXComp->DeactivateImmediate();
				FXComp->DestroyComponent();
			}
			else
			{
				FXComp->Deactivate();
			}
		}
		FXComp = nullptr;
	}
}

UCXGA_FieldPlayerRopeTraversal::UCXGA_FieldPlayerRopeTraversal()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	ActivationRequiredTags.AddTag(CXVIFieldTag::State_CanUseRope);

	ActivationOwnedTags.AddTag(CXVIFieldTag::State_MovingBlock);

	ActivationBlockedTags.AddTag(CXVIFieldTag::State_MovingBlock);
}

bool UCXGA_FieldPlayerRopeTraversal::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const ACXFieldPlayerCharacter* Player = ActorInfo ? Cast<ACXFieldPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	

	return IsValid(Player)&&IsValid(Player->GetRopeTraversalTarget()) && Player->GetRopeTraversalDirection() != ECXRopeTraversalDirection::None 
		&& Player->GetRopeTraversalTarget()->IsFacingRopePoint(Player,Player->GetRopeTraversalDirection())
		&& Player->GetRopeTraversalTarget()->CanShowRopeUI(Player);
}

void UCXGA_FieldPlayerRopeTraversal::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ACXFieldPlayerCharacter* Player = ActorInfo ? Cast<ACXFieldPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!IsValid(Player))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACXRopeTraversalActor* Rope = IsValid(Player) ? Player->GetRopeTraversalTarget() : nullptr;
	if (!IsValid(Rope))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const ECXRopeTraversalDirection Direction = Player->GetRopeTraversalDirection();

	FVector Entry;
	FVector Destination;
	FVector Exit;

	if (!Rope->GetTraversalPoints(Direction, Entry, Destination, Exit))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 서 있던 자리에서 바로 붙으므로 땅 위에서만 시작합니다.
	const UCharacterMovementComponent* StartMovement = Player->GetCharacterMovement();
	if (!IsValid(StartMovement) || !StartMovement->IsMovingOnGround())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TraversalCharacter = Player;
	TraversalRope = Rope;
	TraversalDirection = Direction;

	TraversalEntry = Entry;
	TraversalDestination = Destination;
	TraversalExit = Exit;

	// 1. 로프 진입부터 오른손 이펙트.
	if (IsValid(HandFX))
	{
		HandFXComp = UNiagaraFunctionLibrary::SpawnSystemAttached(HandFX, Player->GetMesh(), HandSocket,
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTargetIncludingScale, true);
	}

	// 접근점까지 걸어가는 단계 없이, 시작 몽타주와 함께 바로 로프에 붙습니다.
	StartMoveToRopeAttachment();

}

void UCXGA_FieldPlayerRopeTraversal::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo))
	{
		return;
	}

	// GAS 내부 잠금 중이면 정리 전체를 나중으로 미룸.
	if (ScopeLockCount > 0)
	{
		WaitingToExecute.Add(
			FPostLockDelegate::CreateUObject(
				this,
				&ThisClass::EndAbility,
				Handle,
				ActorInfo,
				ActivationInfo,
				bReplicateEndAbility,
				bWasCancelled));
		return;
	}
	
	bWaitingForLanding = false;
	bExitLanded = false;
	bExitMontageFinished = false;
	LandingTimeoutTask = nullptr;
	if (ACXFieldPlayerCharacter* Player = TraversalCharacter.Get())
	{
		Player->LandedDelegate.RemoveDynamic(this, &ThisClass::HandleTraversalLanded);
	}

	// 실행 중인 이동 제어를 종료 합니다.
	if (IsValid(TraversalMoveTask))
	{
		TraversalMoveTask->EndTask();
		TraversalMoveTask = nullptr;
	}

	const bool bShouldRestoreMovement = bControlsTraversalMovement;
	bControlsTraversalMovement = false;

	if (ACXFieldPlayerCharacter* Player = TraversalCharacter.Get())
	{
		UCharacterMovementComponent* Movement = Player->GetCharacterMovement();

		// 이번 어빌리티가 변경했고, 현재도 비행 중일 때만 복구.
		if (bShouldRestoreMovement && Movement && Movement->MovementMode == MOVE_Flying)
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_Falling);
		}
	}
	
	// 이번 실행에서 보관함 정보를 비웁니다.
	TraversalCharacter.Reset();
	TraversalRope.Reset();
	TraversalDirection = ECXRopeTraversalDirection::None;
	TraversalEntry = FVector::ZeroVector;
	TraversalDestination = FVector::ZeroVector;
	TraversalExit = FVector::ZeroVector;
	DestinationMoveGoal = FVector::ZeroVector;
	ExitMoveGoal = FVector::ZeroVector;
	AttachmentMoveGoal = FVector::ZeroVector;
	StopFaceRope();

	// 중간에 취소돼도 이펙트가 남지 않고 FOV 원래대로.
	DeactivateRopeFX(HandFXComp);
	RestoreFOV();
	StopRideSound();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

}

void UCXGA_FieldPlayerRopeTraversal::OnTraversalInterrupted()
{
	if (!IsActive())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[RopeAbility] 진입 이동 실패/취소"));

	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, true);
}

void UCXGA_FieldPlayerRopeTraversal::StartMoveToDestination()
{

	if (!IsActive())
	{
		return;
	}

	ACXFieldPlayerCharacter* Player = TraversalCharacter.Get();
	UCharacterMovementComponent* Movement = IsValid(Player) ? Player->GetCharacterMovement() : nullptr;

	if (!IsValid(Movement) || !TraversalRope.IsValid())
	{
		OnTraversalInterrupted();
		return;
	}

	// 진입 이동의 Root Motion을 먼저 정리합니다.
	if (IsValid(TraversalMoveTask))
	{
		TraversalMoveTask->EndTask();
		TraversalMoveTask = nullptr;
	}

	const float HalfHeight = Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	DestinationMoveGoal = TraversalDestination + FVector::UpVector * HalfHeight;

	const float Distance = FVector::Distance(Player->GetActorLocation(), DestinationMoveGoal);

	const float Duration = FMath::Max(Distance / FMath::Max(TraversalSpeed, 1.0f), 0.05f);

	Movement->StopMovementImmediately();
	bControlsTraversalMovement = true;
	Movement->SetMovementMode(MOVE_Flying);

	TraversalMoveTask =
		UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
			this,
			FName(TEXT("RopeTraverse")),
			DestinationMoveGoal,
			Duration,
			false,
			MOVE_Flying,
			true,
			nullptr,
			ERootMotionFinishVelocityMode::SetVelocity,
			FVector::ZeroVector,
			0.0f);

	if (!IsValid(TraversalMoveTask))
	{
		OnTraversalInterrupted();
		return;
	}

	TraversalMoveTask->OnTimedOutAndDestinationReached.AddDynamic(this, &ThisClass::OnDestinationFinished);
	TraversalMoveTask->OnTimedOut.AddDynamic(this, &ThisClass::OnTraversalInterrupted);

	PlayTraversalMontage(GetMontageSet().TraversalMontage, TEXT("RopeTraversalMonatage"));
	TraversalMoveTask->ReadyForActivation();

	// 로프 타는 동안 소리. 도착/종료 때 StopRideSound로 끔.
	if (IsValid(RideSound))
	{
		RideAudioComp = UGameplayStatics::SpawnSoundAttached(RideSound, Player->GetRootComponent());

		// 도착 전에 소리 파일이 먼저 끝나면 뚝 끊기므로, 끝나기 직전에 미리 페이드아웃.
		const float SoundDuration = RideSound->GetDuration();
		if (IsValid(RideAudioComp) && SoundDuration < INDEFINITELY_LOOPING_DURATION)
		{
			const float FadeTime = FMath::Min(0.45f, SoundDuration * 0.4f);
			TWeakObjectPtr<UAudioComponent> WeakComp = RideAudioComp;
			FTimerHandle FadeHandle;
			Player->GetWorldTimerManager().SetTimer(FadeHandle, FTimerDelegate::CreateWeakLambda(this, [this, WeakComp, FadeTime]()
			{
				// 이미 StopRideSound로 끈 경우는 건너뜀.
				if (WeakComp.IsValid() && RideAudioComp == WeakComp.Get())
				{
					WeakComp->FadeOut(FadeTime, 0.0f);
					RideAudioComp = nullptr;
				}
			}), SoundDuration - FadeTime, false);
		}
	}

	// 로프 타는 동안 FOV 넓힘.
	TraversalCamera = Player->FindComponentByClass<UCameraComponent>();
	if (TraversalCamera.IsValid())
	{
		if (!bFOVChanged)
		{
			BaseFOV = TraversalCamera->FieldOfView;
		}
		bFOVChanged = true;
		StartFOVBlend(BaseFOV + RideFOVAdd, FOVBlendInTime);
	}
}

void UCXGA_FieldPlayerRopeTraversal::OnDestinationFinished()
{
	if (!IsActive())
	{
		return;
	}

	ACXFieldPlayerCharacter* Player = TraversalCharacter.Get();

	if (!IsValid(Player) || !TraversalRope.IsValid())
	{
		OnTraversalInterrupted();
		return;
	}

	if (FVector::DistSquared(Player->GetActorLocation(), DestinationMoveGoal) > FMath::Square(10.0f))
	{
		OnTraversalInterrupted();
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[RopeAbility] 밧줄 반대쪽 끝 도착"));

	// 3. 도착하면 손 이펙트·로프 소리 끄고 FOV 원래대로.
	DeactivateRopeFX(HandFXComp);
	RestoreFOV();
	StopRideSound();

	StartMoveToExit();
}

void UCXGA_FieldPlayerRopeTraversal::StartFOVBlend(float TargetFOV, float Duration)
{
	UCameraComponent* Camera = TraversalCamera.Get();
	UWorld* World = GetWorld();
	if (!IsValid(World) || !IsValid(Camera))
	{
		return;
	}

	FOVFrom = Camera->FieldOfView;
	FOVTo = TargetFOV;
	FOVBlendDuration = FMath::Max(Duration, KINDA_SMALL_NUMBER);
	FOVBlendStartTime = World->GetTimeSeconds();

	World->GetTimerManager().SetTimer(FOVBlendTimerHandle, this, &ThisClass::TickFOVBlend, 0.016f, true);
}

void UCXGA_FieldPlayerRopeTraversal::TickFOVBlend()
{
	UWorld* World = GetWorld();
	UCameraComponent* Camera = TraversalCamera.Get();
	if (!IsValid(World) || !IsValid(Camera))
	{
		if (World)
		{
			World->GetTimerManager().ClearTimer(FOVBlendTimerHandle);
		}
		return;
	}

	// 타이머 간격 대신 실제 경과 시간으로 계산해서 프레임이 튀어도 일정하게.
	const float Alpha = FMath::Clamp((World->GetTimeSeconds() - FOVBlendStartTime) / FOVBlendDuration, 0.0f, 1.0f);
	Camera->SetFieldOfView(FMath::InterpEaseInOut(FOVFrom, FOVTo, Alpha, 2.0f));

	if (Alpha >= 1.0f)
	{
		World->GetTimerManager().ClearTimer(FOVBlendTimerHandle);
	}
}

void UCXGA_FieldPlayerRopeTraversal::RestoreFOV()
{
	if (!bFOVChanged)
	{
		return;
	}

	bFOVChanged = false;
	StartFOVBlend(BaseFOV, FOVBlendOutTime);
}

void UCXGA_FieldPlayerRopeTraversal::StartMoveToExit()
{
	bExitLanded = false;
	bExitMontageFinished = false;

	UAbilityTask_PlayMontageAndWait* ExitMontageTask = PlayTraversalMontage(GetMontageSet().ExitMontage, TEXT("RopeExitMontage"), false);
	if (IsValid(ExitMontageTask))
	{
		ExitMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnExitMontageFinished);
		ExitMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnExitMontageFinished);
		ExitMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnExitMontageFinished);
		ExitMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnExitMontageFinished);
	}
	else
	{
		// 착지 몽타주가 없으면 착지만 기다립니다.
		bExitMontageFinished = true;
	}

	if (!IsActive())
	{
		return;
	}

	ACXFieldPlayerCharacter* Player = TraversalCharacter.Get();
	UCharacterMovementComponent* Movement = IsValid(Player) ? Player->GetCharacterMovement() : nullptr;

	if (!IsValid(Movement) || !TraversalRope.IsValid())
	{
		OnTraversalInterrupted();
		return;
	}

	if (IsValid(TraversalMoveTask))
	{
		TraversalMoveTask->EndTask();
		TraversalMoveTask = nullptr;
	}


	const float HalfHeight = Player->GetCapsuleComponent()-> GetScaledCapsuleHalfHeight();

	ExitMoveGoal = TraversalExit + FVector::UpVector * (HalfHeight + 2.0f);

	Movement->StopMovementImmediately();
	bControlsTraversalMovement = true;
	Movement->SetMovementMode(MOVE_Flying);

	UCurveVector* ExitCurvePath = (TraversalDirection == ECXRopeTraversalDirection::Up) ? ExitUpCurvePath.Get() : ExitDownCurvePath.Get();

	TraversalMoveTask =
		UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
			this,
			FName(TEXT("RopeExit")),
			ExitMoveGoal,
			FMath::Max(ExitMoveDuration, 0.05f),
			false,
			MOVE_Flying,
			true,
			ExitCurvePath,
			ERootMotionFinishVelocityMode::SetVelocity,
			FVector::ZeroVector,
			0.0f
		);

	if (!IsValid(TraversalMoveTask))
	{
		OnTraversalInterrupted();
		return;
	}

	TraversalMoveTask->OnTimedOutAndDestinationReached.AddDynamic(this, &ThisClass::OnExitMoveFinished);
	TraversalMoveTask->OnTimedOut.AddDynamic(this, &ThisClass::OnTraversalInterrupted);

	TraversalMoveTask->ReadyForActivation();
}

void UCXGA_FieldPlayerRopeTraversal::OnExitMoveFinished()
{
	if (!IsActive())
	{
		return;
	}

	ACXFieldPlayerCharacter* Player = TraversalCharacter.Get();

	if (!IsValid(Player) || !TraversalRope.IsValid())
	{
		OnTraversalInterrupted();
		return;
	}

	if (FVector::DistSquared(Player->GetActorLocation(), ExitMoveGoal) > FMath::Square(10.0f))
	{
		OnTraversalInterrupted();
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[RopeAbility] Exit 도착"));

	UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
	if (!IsValid(Movement))
	{
		OnTraversalInterrupted();
		return;
	}

	if (IsValid(TraversalMoveTask))
	{
		TraversalMoveTask->EndTask();
		TraversalMoveTask = nullptr;
	}

	Movement->StopMovementImmediately();

	bWaitingForLanding = true;
	Player->LandedDelegate.AddUniqueDynamic(this, &ThisClass::HandleTraversalLanded);

	LandingTimeoutTask = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(LandingTimeOut, 0.1f));

	LandingTimeoutTask->OnFinish.AddDynamic(this, &ThisClass::OnTraversalInterrupted);
	LandingTimeoutTask->ReadyForActivation();

	Movement->SetMovementMode(MOVE_Falling);

}

void UCXGA_FieldPlayerRopeTraversal::HandleTraversalLanded(const FHitResult& Hit)
{
	if (!IsActive() || !bWaitingForLanding)
	{
		return;
	}

	ACXFieldPlayerCharacter* Player = TraversalCharacter.Get();
	UCharacterMovementComponent* Movement = IsValid(Player) ? Player->GetCharacterMovement() : nullptr;
	if (!IsValid(Movement)||!TraversalRope.IsValid()||!Movement->IsWalkable(Hit))
	{
		OnTraversalInterrupted();
		return;
	}

	const float HalfHeight = Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	const FVector FeetLocation = Player->GetActorLocation() - FVector::UpVector * HalfHeight;

	if (FVector::DistSquared(FeetLocation, TraversalExit)>FMath::Square(50.0f))
	{
		OnTraversalInterrupted();
		return;
	}

	bWaitingForLanding = false;
	Player->LandedDelegate.RemoveDynamic(this, &ThisClass::HandleTraversalLanded);

	// 착지했으니 타임아웃은 더 이상 필요 없습니다.
	if (IsValid(LandingTimeoutTask))
	{
		LandingTimeoutTask->EndTask();
		LandingTimeoutTask = nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("[RopeAbility] Exit 착지 완료"));

	// 4. 착지 먼지.
	if (IsValid(LandDustFX))
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, LandDustFX, Hit.ImpactPoint);
	}

	bExitLanded = true;
	TryFinishExit();

}

void UCXGA_FieldPlayerRopeTraversal::OnExitMontageFinished()
{
	if (!IsActive() || bExitMontageFinished)
	{
		return;
	}

	bExitMontageFinished = true;

	UE_LOG(LogTemp, Log, TEXT("[RopeAbility] 착지 몽타주 종료"));

	TryFinishExit();
}

void UCXGA_FieldPlayerRopeTraversal::TryFinishExit()
{
	// 착지와 착지 몽타주가 모두 끝나야 이동 제한을 풉니다.
	if (!IsActive() || !bExitLanded || !bExitMontageFinished)
	{
		return;
	}

	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, false);
}

void UCXGA_FieldPlayerRopeTraversal::StartMoveToRopeAttachment()
{
	if (!IsActive())
	{
		return;
	}

	ACXFieldPlayerCharacter* Player = TraversalCharacter.Get();
	UCharacterMovementComponent* Movement = IsValid(Player) ? Player->GetCharacterMovement() : nullptr;
	if (!IsValid(Movement) || !TraversalRope.IsValid())
	{
		OnTraversalInterrupted();
		return;
	}

	if (IsValid(TraversalMoveTask))
	{
		TraversalMoveTask->EndTask();
		TraversalMoveTask = nullptr;
	}

	const float HalfHeight = Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	AttachmentMoveGoal = TraversalEntry + FVector::UpVector * HalfHeight;

	// 붙는 동안 로프 진행 방향(진입점 -> 도착점)으로 몸을 돌립니다.
	// 로프가 거의 수직이면 진입점을 바라봅니다.
	FVector FaceDirection = (TraversalDestination - TraversalEntry).GetSafeNormal2D();
	if (FaceDirection.IsNearlyZero())
	{
		FaceDirection = (TraversalEntry - Player->GetActorLocation()).GetSafeNormal2D();
	}
	StartFaceRope(FaceDirection);

	Movement->StopMovementImmediately();
	bControlsTraversalMovement = true;
	Movement->SetMovementMode(MOVE_Flying);

	TraversalMoveTask =
		UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
			this,
			FName(TEXT("RopeAttachment")),
			AttachmentMoveGoal,
			FMath::Max(AttachDuration, 0.05f),
			false,
			MOVE_Flying,
			true,
			AttachCurvePath,
			ERootMotionFinishVelocityMode::SetVelocity,
			FVector::ZeroVector,
			0.0f
		);

	if (!IsValid(TraversalMoveTask))
	{
		OnTraversalInterrupted();
		return;
	}

	TraversalMoveTask->OnTimedOutAndDestinationReached.AddDynamic(this, &ThisClass::OnRopeAttachmentFinished);

	TraversalMoveTask->OnTimedOut.AddDynamic(this, &ThisClass::OnTraversalInterrupted);

	PlayTraversalMontage(GetMontageSet().AttachMontage, TEXT("RopeAttachMontage"));
	TraversalMoveTask->ReadyForActivation();

}

void UCXGA_FieldPlayerRopeTraversal::OnRopeAttachmentFinished()
{
	if (!IsActive())
	{
		return;
	}

	ACXFieldPlayerCharacter* Player = TraversalCharacter.Get();

	if (!IsValid(Player) || !TraversalRope.IsValid())
	{
		OnTraversalInterrupted();
		return;
	}

	if (FVector::DistSquared(
		Player->GetActorLocation(),
		AttachmentMoveGoal) > FMath::Square(10.0f))
	{
		OnTraversalInterrupted();
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[RopeAbility] 로프 부착 완료"));

	StartMoveToDestination();
}

void UCXGA_FieldPlayerRopeTraversal::StartFaceRope(const FVector& FaceDirection)
{
	ACXFieldPlayerCharacter* Player = TraversalCharacter.Get();
	UWorld* World = GetWorld();
	if (!IsValid(Player) || !IsValid(World) || FaceDirection.IsNearlyZero())
	{
		return;
	}

	FaceFromRot = Player->GetActorQuat();
	FaceToRot = FaceDirection.ToOrientationQuat();
	FaceStartTime = World->GetTimeSeconds();

	World->GetTimerManager().SetTimer(FaceRopeTimerHandle, this, &ThisClass::TickFaceRope, 0.016f, true);
}

void UCXGA_FieldPlayerRopeTraversal::TickFaceRope()
{
	ACXFieldPlayerCharacter* Player = TraversalCharacter.Get();
	UWorld* World = GetWorld();
	if (!IsValid(Player) || !IsValid(World))
	{
		StopFaceRope();
		return;
	}

	const float Alpha = FMath::Clamp((World->GetTimeSeconds() - FaceStartTime) / FMath::Max(FaceRopeTime, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	const float Eased = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);
	Player->SetActorRotation(FQuat::Slerp(FaceFromRot, FaceToRot, Eased));

	if (Alpha >= 1.0f)
	{
		StopFaceRope();
	}
}

void UCXGA_FieldPlayerRopeTraversal::StopFaceRope()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FaceRopeTimerHandle);
	}
}

UAbilityTask_PlayMontageAndWait* UCXGA_FieldPlayerRopeTraversal::PlayTraversalMontage(UAnimMontage* Montage, FName TaskName, bool bStopWhenAbilityEnds)
{
	if (!IsValid(Montage))
	{
		return nullptr;
	}

	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TaskName, Montage, 1.0f, NAME_None, bStopWhenAbilityEnds);
	Task->ReadyForActivation();
	return Task;
}

void UCXGA_FieldPlayerRopeTraversal::StopRideSound()
{
	if (IsValid(RideAudioComp))
	{
		// 도착 후 착지 동작 동안 자연스럽게 잦아들도록 길게 페이드아웃.
		RideAudioComp->FadeOut(0.45f, 0.0f);
	}
	RideAudioComp = nullptr;
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "CXPlayerGrapplingHookAbility.h"
#include "CXVI/Tags/CXFieldTags.h"
#include "CXVI/Character/Field/CXFieldPlayerCharacter.h"
#include "CXVI/Character/Field/Interactions/CXGrapplingHookTriggerBox.h"


#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "GameFramework/RootMotionSource.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXPlayerGrapplingHookAbility)

namespace
{
    // bImmediate=false : 스폰만 멈추고 남은 파티클은 수명대로 사라짐. (AutoDestroy로 스폰해서 끝나면 자동 제거)
    // bImmediate=true  : 남은 파티클까지 즉시 제거. 수명이 무한/루프인 이펙트는 이걸 써야 꺼짐.
    void DeactivateGrapplingFX(TObjectPtr<UNiagaraComponent>& FXComp, bool bImmediate = false)
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

UCXPlayerGrapplingHookAbility::UCXPlayerGrapplingHookAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    ActivationRequiredTags.AddTag(CXVIFieldTag::State_CanGrapplingHook);
    ActivationOwnedTags.AddTag(CXVIFieldTag::State_MovingBlock);

    ActivationBlockedTags.AddTag(CXVIFieldTag::State_MovingBlock);

}

bool UCXPlayerGrapplingHookAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }

    // 캐릭터의 정보를 가져옴.
    const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    const ACXFieldPlayerCharacter* Character = ActorInfo ? Cast<ACXFieldPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
  

    // 캐릭터의 존재여부, 그래플링 타겟 액터와 착지지점이 있는지 확인 하나라도 존재하지 않으면 실패.
    // 나머지(땅 위·점프·스프린트 등 어빌리티 사용 중·이동 제한·시선)는 UI 표시 조건과 같게 트리거에서 확인.
    const ACXGrapplingHookTriggerBox* TriggerBox = IsValid(Character) ? Character->GetGrapplingTriggerBox() : nullptr;

    return IsValid(Character)
        && IsValid(Character->GetGrapplingTargetActor())
        && IsValid(Character->GetLandingSurfaceActor())
        && IsValid(TriggerBox)
        && TriggerBox->CanShowGrapplingUI(Character);
}

void UCXPlayerGrapplingHookAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    // 그래플링을 하기위해 어빌리티 시스템과 캐릭터를 가져온다.
    UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    ACXFieldPlayerCharacter* Character = ActorInfo ? Cast<ACXFieldPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;

    if (!IsValid(ASC) || !IsValid(Character))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }


    
    // 캐릭터 -> 타깃 방향.
    // 타겟(그래플링) 위치.
    // 착지 지점(LandingSurface).
    AActor* Target = Character->GetGrapplingTargetActor();
    UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
    GrapplingTarget = Target;
    LandingSurface = Character->GetLandingSurfaceActor();
    GrapplingCharacter = Character;
    if (!IsValid(Target) || !IsValid(Movement) || !GrapplingTarget.IsValid() || !LandingSurface.IsValid())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const float CapsuleHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

    // 착지 발판은 위치 표시용이라 실제 바닥과 높이가 다를 수 있어 아래로 트레이스해서 바닥 높이를 찾음.
    FVector LandingFloor = LandingSurface->GetActorLocation();
    {
        FHitResult FloorHit;
        FCollisionQueryParams FloorParams(SCENE_QUERY_STAT(GrapplingLandingFloor), false, Character);
        const FVector TraceStart = LandingFloor + FVector::UpVector * 200.0f;
        const FVector TraceEnd = LandingFloor - FVector::UpVector * 500.0f;

        if (GetWorld()->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_Visibility, FloorParams))
        {
            LandingFloor = FloorHit.ImpactPoint;
        }
    }

    // 캐릭터의 액터 좌표는 캡슐 중심이므로, 발 위치에 캡슐 반높이를 더해야됨.
    GrapplingGoal = LandingFloor + FVector::UpVector * (CapsuleHalfHeight + 2.0f);

    // 필요한 몽타주 Section이 준비됐는지 검사.
    const bool bHasSection =
        IsValid(GrapplingMontage) &&
        GrapplingMontage->GetSectionIndex(FName(TEXT("Start"))) != INDEX_NONE &&
        GrapplingMontage->GetSectionIndex(FName(TEXT("Flight"))) != INDEX_NONE &&
        GrapplingMontage->GetSectionIndex(FName(TEXT("Land"))) != INDEX_NONE;

    if (!bHasSection)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }


    // 준비 단계 시작.
    GrapplingPhase = ECXGrapplingPhase::Preparing;
    bControlsGrapplingMovement = false;

    Movement->StopMovementImmediately();

    Character->HideCable();

    Character->LandedDelegate.AddUniqueDynamic(this, &ThisClass::HandleGrapplingLanded);

    // 케이블 표시 이벤트 대기
    auto* ShowCableWait = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, CXVIFieldTag::Event_Field_ShowCable, nullptr, true, true);
    ShowCableWait->EventReceived.AddDynamic(this, &ThisClass::OnShowCableEvent);

    ShowCableWait->ReadyForActivation();

    // 1. 훅을 걸 때 오른손 이펙트.
    if (IsValid(HandFX))
    {
        HandFXComp = UNiagaraFunctionLibrary::SpawnSystemAttached(HandFX, Character->GetMesh(), HandSocket,
            FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTargetIncludingScale, true);
    }

    // 비행 시작 이벤트 대기.
    auto* StartMoveWait = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, CXVIFieldTag::Event_Field_StartGrapplingHook, nullptr, true, true);
    StartMoveWait->EventReceived.AddDynamic(this, &ThisClass::OnStartMoveEvent);
    StartMoveWait->ReadyForActivation();

    // Notify 누락이나 착지 실패 등으로 계속 남아 있는 상황 제한 시간.
    auto* TimeoutTask = UAbilityTask_WaitDelay::WaitDelay(this, GrapplingTimeOut);
    TimeoutTask->OnFinish.AddDynamic(this, &ThisClass::OnGrapplingInterrupted);
    TimeoutTask->ReadyForActivation();

    // 팔을 뻗는 동안 착지 방향으로 몸(Yaw)을 돌림. 몽타주는 바로 재생.
    StartFaceGoal();

    StartGrapplingMontage();

}

void UCXPlayerGrapplingHookAbility::EndAbility(
    const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo, 
    const FGameplayAbilityActivationInfo ActivationInfo, 
    bool bReplicateEndAbility, 
    bool bWasCancelled)
{
    if (!IsEndAbilityValid(Handle, ActorInfo))
    {
        return;
    }

    // GAS 내부 처리 중이면 정리 전체를 나중으로 미룸.
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
                bWasCancelled
            )
        );

        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(GrapplingAirHoldTimerHandle);
    }

    // 멈춰 둔 몽타주가 남지 않도록 재생을 되돌림.
    ResumeGrapplingMontage();

    // 실행 중인 좌표 이동 제어 종료.
    if (IsValid(GrapplingMoveTask))
    {
        GrapplingMoveTask->EndTask();
        GrapplingMoveTask = nullptr;
    }

    const bool bShouldRestoreMovement = bControlsGrapplingMovement;

    bControlsGrapplingMovement = false;

    ACXFieldPlayerCharacter* Character = ActorInfo ? Cast<ACXFieldPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;


    if (IsValid(Character))
    {
        Character->LandedDelegate.RemoveDynamic(this, &ThisClass::HandleGrapplingLanded);
        UCharacterMovementComponent* Movement = Character->GetCharacterMovement();

        if (bShouldRestoreMovement && IsValid(Movement) && Movement->MovementMode == MOVE_Flying)
        {
            Movement->StopMovementImmediately();

            // 공중 취소 시 자연스럽게 떨어지도록 복귀.
            Movement->SetMovementMode(MOVE_Falling);
        }

        Character->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);

    }

    // 회전 타이머 정리 및 비행 중 고정한 회전 설정 복구.
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FaceGoalTimerHandle);
    }
    RestoreFlightRotation();

    // 취소·타임아웃으로 끝나도 이펙트가 몸에 남지 않게 정리.
    DeactivateGrapplingFX(HandFXComp);
    StopFlightSound();

    UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

    GrapplingPhase = ECXGrapplingPhase::None;
    GrapplingCharacter.Reset();
    GrapplingTarget.Reset();
    LandingSurface.Reset();
    GrapplingGoal = FVector::ZeroVector;

    if (bFOVChanged)
    {
        bFOVChanged = false;
        StartFOVBlend(BaseFOV, FOVBlendOutTime);
    }

    // 취소·타임아웃 등으로 중간에 끝나도 카메라 거리 원래대로.
    RestoreArmLength();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UCXPlayerGrapplingHookAbility::HandleGrapplingLanded(const FHitResult& Hit)
{
    if (!IsActive() || GrapplingPhase!=ECXGrapplingPhase::Flying)
    {
        return;
    }

    // 착지 발판에 충돌이 없을 수 있어 액터 대신 착지 지점이 발판 영역 안인지 확인.
    bool bCorrectLanding = LandingSurface.IsValid() && Hit.GetActor() == LandingSurface.Get();

    if (!bCorrectLanding && LandingSurface.IsValid())
    {
        FVector SurfaceOrigin;
        FVector SurfaceExtent;
        LandingSurface->GetActorBounds(false, SurfaceOrigin, SurfaceExtent, true);

        const FVector Offset = Hit.ImpactPoint - SurfaceOrigin;
        bCorrectLanding =
            FMath::Abs(Offset.X) <= SurfaceExtent.X + 50.0f &&
            FMath::Abs(Offset.Y) <= SurfaceExtent.Y + 50.0f &&
            FMath::Abs(Offset.Z) <= SurfaceExtent.Z + 100.0f;
    }

    if (!bCorrectLanding)
    {
        OnGrapplingInterrupted();
        return;
    }

    const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();

    UAbilitySystemComponent* ASC = Info ? Info->AbilitySystemComponent.Get() : nullptr;

    if (!IsValid(ASC) || !ASC->IsAnimatingAbility(this) || ASC->GetCurrentMontage() != GrapplingMontage.Get())
    {
        OnGrapplingInterrupted();
        return;
    }

    GrapplingPhase = ECXGrapplingPhase::Landing;

    if (bFOVChanged)
    {
        bFOVChanged = false;
        StartFOVBlend(BaseFOV, FOVBlendOutTime);
    }


    if (IsValid(LandDustFX))
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, LandDustFX, Hit.ImpactPoint);
    }

    // 착지하면 비행 소리 끄기.
    StopFlightSound();

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(GrapplingAirHoldTimerHandle);
    }

    ResumeGrapplingMontage();

    ASC->CurrentMontageJumpToSection(FName(TEXT("Land")));
}

void UCXPlayerGrapplingHookAbility::OnGrapplingInterrupted()
{
    if (IsActive())
    {
        EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, true);
    }
}

void UCXPlayerGrapplingHookAbility::OnGrapplingMontageCompleted()
{
    if (!IsActive())
    {
        return;
    }

    if (GrapplingPhase != ECXGrapplingPhase::Landing)
    {
        OnGrapplingInterrupted();
        return;
    }

    EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, false);
}

void UCXPlayerGrapplingHookAbility::OnGrapplingMoveReached()
{
    if (!IsActive() || GrapplingPhase != ECXGrapplingPhase::Flying)
    {
        return;
    }


    ACXFieldPlayerCharacter* Character = GrapplingCharacter.Get();
    if (!IsValid(Character))
    {
        OnGrapplingInterrupted();
        return;
    }
    UCharacterMovementComponent* Movement = Character->GetCharacterMovement();

    if (!IsValid(Movement) || !LandingSurface.IsValid())
    {
        OnGrapplingInterrupted();
        return;
    }
    

    // 이동 제어 종료.
    if (GrapplingMoveTask)
    {
        GrapplingMoveTask->EndTask();
        GrapplingMoveTask = nullptr;
    }

    Movement->StopMovementImmediately();


    // 최종 위치를 정확히 맞춤. 충돌 검사는 유지.
    //FHitResult SweepHit;
    //Character->SetActorLocation(GrapplingGoal, true, &SweepHit);

    //if (FVector::DistSquared(Character->GetActorLocation(), GrapplingGoal) > FMath::Square(2.0f))
    //{
    //    OnGrapplingInterrupted();
    //    return;
    //}

    // 발판 바로 위에서 실제 바닥 착지를 처리.
    Movement->SetMovementMode(MOVE_Falling);
}

void UCXPlayerGrapplingHookAbility::StartGrapplingMontage()
{
    // 재생 실패 롤백에서 능력이 종료됐을 수도 있음.
    if (!IsActive())
    {
        return;
    }

    // 몽타주 재생.
    UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, GrapplingMontage, 1.0f, FName(TEXT("Start")), true);

    MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnGrapplingInterrupted);

    MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnGrapplingInterrupted);

    MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnGrapplingMontageCompleted);

    MontageTask->ReadyForActivation();

}

void UCXPlayerGrapplingHookAbility::OnShowCableEvent(FGameplayEventData Payload)
{
    if (!IsActive() || GrapplingPhase != ECXGrapplingPhase::Preparing)
    {
        return;
    }

    ACXFieldPlayerCharacter* Character = GrapplingCharacter.Get();

    if (!IsValid(Character) || !GrapplingTarget.IsValid())
    {
        OnGrapplingInterrupted();
        return;
    }

    Character->ShowCable(GrapplingTarget.Get());

    // 1. 케이블이 나갈 때 소리.
    if (IsValid(CableSound))
    {
        UGameplayStatics::SpawnSoundAttached(CableSound, Character->GetMesh(), HandSocket);
    }

    // 팔 뻗을 때 카메라 멀어짐.
    GrapplingCameraBoom = Character->FindComponentByClass<USpringArmComponent>();
    if (GrapplingCameraBoom.IsValid())
    {
        if (!bArmChanged)
        {
            BaseArmLength = GrapplingCameraBoom->TargetArmLength;
        }
        bArmChanged = true;
        StartArmBlend(BaseArmLength + ArmPushOutDistance, ArmPushOutTime);
    }
}

void UCXPlayerGrapplingHookAbility::OnStartMoveEvent(FGameplayEventData Payload)
{
    if (!IsActive() || GrapplingPhase != ECXGrapplingPhase::Preparing)
    {
        return;
    }

    ACXFieldPlayerCharacter* Character = GrapplingCharacter.Get();


	UCharacterMovementComponent* Movement = IsValid(Character) ? Character->GetCharacterMovement() : nullptr;


    const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();

	UAbilitySystemComponent* ASC = Info ? Info->AbilitySystemComponent.Get() : nullptr;


    if (!IsValid(Movement) || !IsValid(ASC) || !GrapplingTarget.IsValid() || !LandingSurface.IsValid())
    {
        OnGrapplingInterrupted();
        return;
    }

    if (!ASC->IsAnimatingAbility(this) || ASC->GetCurrentMontage() != GrapplingMontage.Get())
    {
        OnGrapplingInterrupted();
        return;
    }

    GrapplingPhase = ECXGrapplingPhase::Flying;

    // 2. 날아가는 동안 몸 전체 이펙트, 손 이펙트는 끄기.
    DeactivateGrapplingFX(HandFXComp);

    // 2. 날아가는 동안 소리. 착지/종료 때 StopFlightSound로 끔.
    if (IsValid(FlightSound))
    {
        FlightAudioComp = UGameplayStatics::SpawnSoundAttached(FlightSound, Character->GetRootComponent());

        // 착지 전에 소리 파일이 먼저 끝나면 뚝 끊기므로, 끝나기 직전에 미리 페이드아웃.
        const float SoundDuration = FlightSound->GetDuration();
        if (IsValid(FlightAudioComp) && SoundDuration < INDEFINITELY_LOOPING_DURATION)
        {
            const float FadeTime = FMath::Min(0.45f, SoundDuration * 0.4f);
            TWeakObjectPtr<UAudioComponent> WeakComp = FlightAudioComp;
            FTimerHandle FadeHandle;
            Character->GetWorldTimerManager().SetTimer(FadeHandle, FTimerDelegate::CreateWeakLambda(this, [this, WeakComp, FadeTime]()
            {
                // 이미 StopFlightSound로 끈 경우는 건너뜀.
                if (WeakComp.IsValid() && FlightAudioComp == WeakComp.Get())
                {
                    WeakComp->FadeOut(FadeTime, 0.0f);
                    FlightAudioComp = nullptr;
                }
            }), SoundDuration - FadeTime, false);
        }
    }

    GrapplingCamera = Character->FindComponentByClass<UCameraComponent>();
    if (GrapplingCamera.IsValid())
    {
        if (!bFOVChanged)
        {
            BaseFOV = GrapplingCamera->FieldOfView;
        }
        bFOVChanged = true;
        StartFOVBlend(BaseFOV + FlightFOVAdd, FOVBlendInTime);
    }

    // 날아갈 때 카메라를 원래 거리로 복귀.
    if (bArmChanged && GrapplingCameraBoom.IsValid())
    {
        StartArmBlend(BaseArmLength, ArmReturnTime);
    }


    ASC->CurrentMontageJumpToSection(FName(TEXT("Flight")));

    // 달리던 속도를 제거.
    Movement->StopMovementImmediately();

    // 회전이 덜 끝났으면 최종 방향으로 맞추고, 비행 중에는 입력/이동 방향으로 몸이 돌지 않게 고정.
    FinishFaceGoal();
    LockFlightRotation();

    // 이동 모드는 어빌리티가 직접 관리.
    bControlsGrapplingMovement = true;
    Movement->SetMovementMode(MOVE_Flying);

    Character->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);

    GrapplingMoveTask =
        UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
            this,
            FName(TEXT("GrapplingMove")),
            GrapplingGoal,
            GrapplingDuration,     
            false,                                          // 이동 모드는 위에서 직접 실행.
            MOVE_Flying,
            false,                                          // 경로상의 목표 위치를 따라잡도록 보정.
            GrapplingPathCurve,
            ERootMotionFinishVelocityMode::SetVelocity,
            FVector::ZeroVector,
            0.0f
        );
    if (!GrapplingMoveTask)
    {
        OnGrapplingInterrupted();
        return;
    }

    GrapplingMoveTask->OnTimedOutAndDestinationReached.AddDynamic(this, &ThisClass::OnGrapplingMoveReached);

    GrapplingMoveTask->OnTimedOut.AddDynamic(this, &ThisClass::OnGrapplingInterrupted);

    GrapplingMoveTask->ReadyForActivation();

    // Flight Section이 끝나기 직전에 아직 공중이면 착지 전까지 자세를 유지.
    // Section 길이는 몽타주 타임라인 기준이라 RateScale로 실제 재생 시간을 구함.
    const int32 FlightIndex = GrapplingMontage->GetSectionIndex(FName(TEXT("Flight")));
    const float FlightLength = GrapplingMontage->GetSectionLength(FlightIndex) / FMath::Max(GrapplingMontage->RateScale, KINDA_SMALL_NUMBER);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            GrapplingAirHoldTimerHandle,
            this,
            &ThisClass::HoldFlightPoseUntilLanded,
            FMath::Max(FlightLength - 1.0f / 60.0f, 0.01f),
            false);
    }

}

void UCXPlayerGrapplingHookAbility::StartFOVBlend(float TargetFOV, float Duration)
{
    UCameraComponent* Camera = GrapplingCamera.Get();
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

void UCXPlayerGrapplingHookAbility::TickFOVBlend()
{
    UWorld* World = GetWorld();
    UCameraComponent* Camera = GrapplingCamera.Get();
    if (!IsValid(World) || !IsValid(Camera))
    {
        if (World)
        { 
            World->GetTimerManager().ClearTimer(FOVBlendTimerHandle); 
        }
        return;
    }

    const float Alpha = FMath::Clamp((World->GetTimeSeconds() - FOVBlendStartTime) / FOVBlendDuration, 0.0f, 1.0f);
    Camera->SetFieldOfView(FMath::InterpEaseInOut(FOVFrom, FOVTo, Alpha, 2.0f));

    if (Alpha >= 1.0f)
    {
        World->GetTimerManager().ClearTimer(FOVBlendTimerHandle);
    }
}

void UCXPlayerGrapplingHookAbility::StartArmBlend(float TargetLength, float Duration, float NextTarget, float NextDuration)
{
    USpringArmComponent* Boom = GrapplingCameraBoom.Get();
    UWorld* World = GetWorld();
    if (!IsValid(World) || !IsValid(Boom))
    {
        return;
    }

    ArmFrom = Boom->TargetArmLength;
    ArmTo = TargetLength;
    ArmBlendDuration = FMath::Max(Duration, KINDA_SMALL_NUMBER);
    ArmBlendStartTime = World->GetTimeSeconds();
    ArmNextTarget = NextTarget;
    ArmNextDuration = NextDuration;

    World->GetTimerManager().SetTimer(ArmBlendTimerHandle, this, &ThisClass::TickArmBlend, 0.016f, true);
}

void UCXPlayerGrapplingHookAbility::TickArmBlend()
{
    UWorld* World = GetWorld();
    USpringArmComponent* Boom = GrapplingCameraBoom.Get();
    if (!IsValid(World) || !IsValid(Boom))
    {
        if (World)
        {
            World->GetTimerManager().ClearTimer(ArmBlendTimerHandle);
        }
        return;
    }

    const float Alpha = FMath::Clamp((World->GetTimeSeconds() - ArmBlendStartTime) / ArmBlendDuration, 0.0f, 1.0f);
    Boom->TargetArmLength = FMath::InterpEaseInOut(ArmFrom, ArmTo, Alpha, 2.0f);

    if (Alpha < 1.0f)
    {
        return;
    }

    World->GetTimerManager().ClearTimer(ArmBlendTimerHandle);

    // 이어서 갈 목표가 있으면 바로 다음 보간 시작.
    if (ArmNextTarget >= 0.0f)
    {
        const float Next = ArmNextTarget;
        const float NextTime = ArmNextDuration;
        StartArmBlend(Next, NextTime);
        return;
    }

    // 원래 거리까지 돌아왔으면 상태 정리.
    if (FMath::IsNearlyEqual(Boom->TargetArmLength, BaseArmLength))
    {
        bArmChanged = false;
    }
}

void UCXPlayerGrapplingHookAbility::RestoreArmLength()
{
    if (!bArmChanged || !GrapplingCameraBoom.IsValid())
    {
        bArmChanged = false;
        return;
    }

    // 이미 원래 거리로 돌아가는 중이면 그대로 둠.
    const bool bReturning = ArmNextTarget < 0.0f && FMath::IsNearlyEqual(ArmTo, BaseArmLength);
    if (!bReturning)
    {
        StartArmBlend(BaseArmLength, ArmReturnTime);
    }
}

void UCXPlayerGrapplingHookAbility::HoldFlightPoseUntilLanded()
{
    if (!IsActive() || GrapplingPhase != ECXGrapplingPhase::Flying)
    {
        return;
    }

    ACXFieldPlayerCharacter* Character = GrapplingCharacter.Get();
    UAnimInstance* AnimInstance = IsValid(Character) && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;

    if (!IsValid(AnimInstance) || !AnimInstance->Montage_IsPlaying(GrapplingMontage))
    {
        return;
    }

    // 타이머 오차로 Land Section에 들어가지 않도록 Flight 마지막 프레임에 고정.
    float FlightStart = 0.0f;
    float FlightEnd = 0.0f;
    GrapplingMontage->GetSectionStartAndEndTime(GrapplingMontage->GetSectionIndex(FName(TEXT("Flight"))), FlightStart, FlightEnd);

    AnimInstance->Montage_SetPosition(GrapplingMontage, FMath::Max(FlightEnd - KINDA_SMALL_NUMBER * 10.0f, FlightStart));
    AnimInstance->Montage_Pause(GrapplingMontage);
    bGrapplingMontagePaused = true;
}

void UCXPlayerGrapplingHookAbility::ResumeGrapplingMontage()
{
    if (!bGrapplingMontagePaused)
    {
        return;
    }

    bGrapplingMontagePaused = false;

    ACXFieldPlayerCharacter* Character = GrapplingCharacter.Get();
    UAnimInstance* AnimInstance = IsValid(Character) && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;

    if (IsValid(AnimInstance))
    {
        AnimInstance->Montage_Resume(GrapplingMontage);
    }
}

void UCXPlayerGrapplingHookAbility::StartFaceGoal()
{
    ACXFieldPlayerCharacter* Character = GrapplingCharacter.Get();
    UWorld* World = GetWorld();
    if (!IsValid(Character) || !IsValid(World))
    {
        return;
    }

    // 착지 지점 방향의 수평 성분만 사용 (위아래로 기울지 않게).
    const FVector FaceDirection = (GrapplingGoal - Character->GetActorLocation()).GetSafeNormal2D();
    if (FaceDirection.IsNearlyZero())
    {
        return;
    }

    FaceFromRot = Character->GetActorQuat();
    FaceToRot = FaceDirection.ToOrientationQuat();
    FaceStartTime = World->GetTimeSeconds();

    World->GetTimerManager().SetTimer(FaceGoalTimerHandle, this, &ThisClass::TickFaceGoal, 0.016f, true);
}

void UCXPlayerGrapplingHookAbility::TickFaceGoal()
{
    ACXFieldPlayerCharacter* Character = GrapplingCharacter.Get();
    UWorld* World = GetWorld();
    if (!IsValid(Character) || !IsValid(World))
    {
        if (World)
        {
            World->GetTimerManager().ClearTimer(FaceGoalTimerHandle);
        }
        return;
    }

    const float Alpha = FMath::Clamp((World->GetTimeSeconds() - FaceStartTime) / FMath::Max(FaceGoalTime, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
    Character->SetActorRotation(FQuat::Slerp(FaceFromRot, FaceToRot, FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f)));

    if (Alpha >= 1.0f)
    {
        World->GetTimerManager().ClearTimer(FaceGoalTimerHandle);
    }
}

void UCXPlayerGrapplingHookAbility::FinishFaceGoal()
{
    UWorld* World = GetWorld();
    if (!IsValid(World) || !World->GetTimerManager().IsTimerActive(FaceGoalTimerHandle))
    {
        return;
    }

    World->GetTimerManager().ClearTimer(FaceGoalTimerHandle);

    if (ACXFieldPlayerCharacter* Character = GrapplingCharacter.Get())
    {
        Character->SetActorRotation(FaceToRot);
    }
}

void UCXPlayerGrapplingHookAbility::LockFlightRotation()
{
    ACXFieldPlayerCharacter* Character = GrapplingCharacter.Get();
    UCharacterMovementComponent* Movement = IsValid(Character) ? Character->GetCharacterMovement() : nullptr;
    if (!IsValid(Movement) || bRotationLocked)
    {
        return;
    }

    bSavedOrientRotationToMovement = Movement->bOrientRotationToMovement;
    bSavedUseControllerDesiredRotation = Movement->bUseControllerDesiredRotation;

    Movement->bOrientRotationToMovement = false;
    Movement->bUseControllerDesiredRotation = false;
    bRotationLocked = true;
}

void UCXPlayerGrapplingHookAbility::RestoreFlightRotation()
{
    if (!bRotationLocked)
    {
        return;
    }
    bRotationLocked = false;

    ACXFieldPlayerCharacter* Character = GrapplingCharacter.Get();
    UCharacterMovementComponent* Movement = IsValid(Character) ? Character->GetCharacterMovement() : nullptr;
    if (IsValid(Movement))
    {
        Movement->bOrientRotationToMovement = bSavedOrientRotationToMovement;
        Movement->bUseControllerDesiredRotation = bSavedUseControllerDesiredRotation;
    }
}

void UCXPlayerGrapplingHookAbility::StopFlightSound()
{
    if (IsValid(FlightAudioComp))
    {
        // 착지 동작 동안 자연스럽게 잦아들도록 길게 페이드아웃.
        FlightAudioComp->FadeOut(0.45f, 0.0f);
    }
    FlightAudioComp = nullptr;
}

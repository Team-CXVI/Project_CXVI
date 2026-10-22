// Fill out your copyright notice in the Description page of Project Settings.


#include "CXPlayerGrapplingHookAbility.h"
#include "CXVI/Tags/CXFieldTags.h"
#include "CXVI/Character/Field/CXFieldPlayerCharacter.h"


#include "Animation/AnimMontage.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "GameFramework/RootMotionSource.h"


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
  

    // 캐릭터의 존재여부, 어트리뷰트.스태미나의 여분, 그래플링 타겟 액터와 착지지점이 있는지 확인 하나라도 존재하지 않으면 실패.
    return IsValid(Character)
        && IsValid(Character->GetGrapplingTargetActor())
        && IsValid(Character->GetLandingSurfaceActor());
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

    // 캐릭터의 액터 좌표는 캡슐 중심이므로, 발 위치에 캡슐 반높이를 더해야됨.
    GrapplingGoal = LandingSurface->GetActorLocation() + FVector::UpVector * (CapsuleHalfHeight + 2.0f);

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

    // 비행 시작 이벤트 대기.
    auto* StartMoveWait = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, CXVIFieldTag::Event_Field_StartGrapplingHook, nullptr, true, true);
    StartMoveWait->EventReceived.AddDynamic(this, &ThisClass::OnStartMoveEvent);
    StartMoveWait->ReadyForActivation();

    // Notify 누락이나 착지 실패 등으로 계속 남아 있는 상황 제한 시간.
    auto* TimeoutTask = UAbilityTask_WaitDelay::WaitDelay(this, GrapplingTimeOut);
    TimeoutTask->OnFinish.AddDynamic(this, &ThisClass::OnGrapplingInterrupted);
    TimeoutTask->ReadyForActivation();

    // 몽타주 재생.
    UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, GrapplingMontage, 1.0f, FName(TEXT("Start")), true);

    MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnGrapplingInterrupted);

    MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnGrapplingInterrupted);

    MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnGrapplingMontageCompleted);

    MontageTask->ReadyForActivation();



    // 재생 실패 롤백에서 능력이 종료됐을 수도 있음.
    if (!IsActive())
    {
        return;
    }


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

    }

    UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

    GrapplingPhase = ECXGrapplingPhase::None;
    GrapplingCharacter.Reset();
    GrapplingTarget.Reset();
    LandingSurface.Reset();
    GrapplingGoal = FVector::ZeroVector;

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UCXPlayerGrapplingHookAbility::HandleGrapplingLanded(const FHitResult& Hit)
{
    if (!IsActive() || GrapplingPhase!=ECXGrapplingPhase::Flying)
    {
        return;
    }

    const bool bCorrectLanding = LandingSurface.IsValid() && Hit.GetActor() == LandingSurface.Get();

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


    ASC->CurrentMontageJumpToSection(FName(TEXT("Flight")));

    // 달리던 속도를 제거.
    Movement->StopMovementImmediately();

    // 이동 모드는 어빌리티가 직접 관리.
    bControlsGrapplingMovement = true;
    Movement->SetMovementMode(MOVE_Flying);

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
  
}

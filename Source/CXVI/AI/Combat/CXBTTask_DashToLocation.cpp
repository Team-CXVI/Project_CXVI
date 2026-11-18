// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Combat/CXBTTask_DashToLocation.h"
#include "Character/Combat/CXCombatCharacterBase.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "MotionWarpingComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXBTTask_DashToLocation)

namespace
{
	const FName DashWarpTargetName(TEXT("DashDestination"));
}

UCXBTTask_DashToLocation::UCXBTTask_DashToLocation()
{
	NodeName = TEXT("DashLocation");

	// 각 캐릭터의 BT가 대시 실행 상태를 따로 보관한다.
	bCreateNodeInstance = true;
	DestinationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UCXBTTask_DashToLocation, DestinationKey));
}

EBTNodeResult::Type UCXBTTask_DashToLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* Controller = OwnerComp.GetAIOwner();
    ACXCombatCharacterBase* Character = Controller ? Cast<ACXCombatCharacterBase>(Controller->GetPawn()) : nullptr;
    const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    const FName KeyName = DestinationKey.SelectedKeyName;

    if (!Character || !Blackboard || !Blackboard->IsVectorValueSet(KeyName) || !Character->GetDashMontage())
    {
        return EBTNodeResult::Failed;
    }

    DashMontage = Character->GetDashMontage();
    Destination = Blackboard->GetValueAsVector(KeyName);
    const FVector Delta = Destination - Character->GetActorLocation();

    if (Delta.Size2D() <= ArrivalTolerance)
    {
        return EBTNodeResult::Succeeded;
    }

    UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
    UMotionWarpingComponent* Warping = Character->FindComponentByClass<UMotionWarpingComponent>();

    if (!AnimInstance || !Warping)
    {
        return EBTNodeResult::Failed;
    }

    Controller->StopMovement();
    Character->SetActorRotation(FRotator(0.f, Delta.Rotation().Yaw, 0.f));

    // 몽타주의 Motion Warping Notify에 지정한 이름과 일치해야 한다.
    Warping->AddOrUpdateWarpTargetFromLocation(DashWarpTargetName, Destination);

    if (AnimInstance->Montage_Play(
        DashMontage, 1.f, EMontagePlayReturnType::MontageLength, 0.f, false) <= 0.f)
    {
        Warping->RemoveWarpTarget(DashWarpTargetName);
        return EBTNodeResult::Failed;
    }

    ActiveTree = &OwnerComp;
    DashCharacter = Character;
    DashAnimInstance = AnimInstance;
    MotionWarping = Warping;

    // 취소된 이전 대시의 종료 콜백이 새 대시를 끝내지 않도록 구분한다.
    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &UCXBTTask_DashToLocation::HandleMontageEnded, ++ActiveDashId);
    AnimInstance->Montage_SetBlendingOutDelegate(EndDelegate, DashMontage);

    return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UCXBTTask_DashToLocation::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UAnimInstance* AnimInstance = DashAnimInstance.Get();

    // 몽타주를 정지하기 전에 실행 상태를 지워 종료 콜백이 BT를 진행시키지 않게 한다.
    ClearDashState();

    if (AnimInstance && DashMontage)
    {
        AnimInstance->Montage_Stop(0.1f, DashMontage);
    }

    return EBTNodeResult::Aborted;
}

void UCXBTTask_DashToLocation::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted, uint32 DashId)
{
    UBehaviorTreeComponent* Tree = ActiveTree.Get();
    if (!Tree || Montage != DashMontage || DashId != ActiveDashId)
    {
        return;
    }

    const ACharacter* Character = DashCharacter.Get();
    const bool bArrived = Character &&
        FVector::Dist2D(Character->GetActorLocation(), Destination) <= ArrivalTolerance;

    ClearDashState();

    FinishLatentTask(*Tree, !bInterrupted && bArrived
        ? EBTNodeResult::Succeeded
        : EBTNodeResult::Failed);
}

void UCXBTTask_DashToLocation::ClearDashState()
{
    if (UMotionWarpingComponent* Warping = MotionWarping.Get())
    {
        Warping->RemoveWarpTarget(DashWarpTargetName);
    }

    ActiveTree.Reset();
    DashCharacter.Reset();
    DashAnimInstance.Reset();
}



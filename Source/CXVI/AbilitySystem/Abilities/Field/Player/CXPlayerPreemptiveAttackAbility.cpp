// Fill out your copyright notice in the Description page of Project Settings.


#include "CXPlayerPreemptiveAttackAbility.h"
#include "CXVI/Character/Field/CXFieldPlayerCharacter.h"
#include "CXVI/Tags/CXFieldTags.h"

#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXPlayerPreemptiveAttackAbility)

UCXPlayerPreemptiveAttackAbility::UCXPlayerPreemptiveAttackAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UCXPlayerPreemptiveAttackAbility::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(ActorInfo->AvatarActor.Get());
	
	// #Todo: 몬스터의 추적 상태도 넣어야 함.
	return Player && ASC;
}

void UCXPlayerPreemptiveAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(ActorInfo->AvatarActor.Get());

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		// 아니면 종료.
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 슬로우 모션 이벤트 발생.
	UAbilityTask_WaitGameplayEvent* SlowMotionTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			CXVIFieldTag::Event_Field_EnterSlowMotion,
			nullptr, // 이 어빌리티의 ASC에서 기다림.
			true,	 // 한 번만 받음.
			true);	 // 정확히 같은 태그만 받음.

	SlowMotionTask->EventReceived.AddDynamic(this, &ThisClass::OnSlowMotionEvent);

	SlowMotionTask->ReadyForActivation();

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AssassinaionMontage, 1.0f, NAME_None, true);

	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnAssassinationInterrupted);

	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnAssassinationInterrupted);

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnAssassinationCompleted);

	MontageTask->ReadyForActivation();

	// 재생 실패 롤백에서 능력이 종료됐을 수도 있음.
	if (!IsActive())
	{
		return;
	}

}

void UCXPlayerPreemptiveAttackAbility::EndAbility(
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

	if (bAppliedSlowMotion)
	{
		bAppliedSlowMotion = false;

		UGameplayStatics::SetGlobalTimeDilation(this, OriginalTimeDilation);
	}


	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UCXPlayerPreemptiveAttackAbility::OnSlowMotionEvent(FGameplayEventData Payload)
{
	SlowMotion();
}

void UCXPlayerPreemptiveAttackAbility::SlowMotion()
{
	if (!IsActive() || bAppliedSlowMotion)
	{
		return;
	}
	OriginalTimeDilation = UGameplayStatics::GetGlobalTimeDilation(this);
	const float SlowRate = FMath::Clamp(SlowMotionRate, 0.01f, 1.0f);
	bAppliedSlowMotion = true;
	UGameplayStatics::SetGlobalTimeDilation(this, OriginalTimeDilation * SlowRate);
}

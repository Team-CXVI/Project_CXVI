// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/AbilitySystem/Abilities/CXGA_Fire.h"

#include "CXVI/Tags/CXGameplayTags.h"

#include "CXVI/Character/Combat/CXCombatCharacterPlayer.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGA_Fire)

UCXGA_Fire::UCXGA_Fire()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(CXVIGameplayTags::Ability_Fire);

	SetAssetTags(Tags);
}

bool UCXGA_Fire::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	bool Result = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);

	ACXCombatCharacterPlayer* CombatCharacter = Cast<ACXCombatCharacterPlayer>(ActorInfo->AvatarActor.Get());
	if (!CombatCharacter)
	{
		Result = false;
		return Result;
	}

	Result = CombatCharacter->GetIsAimMode();

	return Result;
}

void UCXGA_Fire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!AbilityMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // 마지막 인자 true -> 취소 됐다
		return;
	}

	RegisterMontageTask();

	ACXCombatCharacterPlayer* CombatCharacter = Cast<ACXCombatCharacterPlayer>(ActorInfo->AvatarActor.Get());
	if (!CombatCharacter)
	{
		return;
	}

	FHitResult Hit;

	CombatCharacter->PerformAimTrace(Hit);
}

void UCXGA_Fire::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UCXGA_Fire::OnAttackCompleted()
{
	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, false); // 마지막 인자가 false -> 정상 종료
}

void UCXGA_Fire::OnAttackInterrupted()
{
	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, true);
}

void UCXGA_Fire::RegisterMontageTask()
{
	// 몽타주를 재생할 Task
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,					// 사용할 어빌리티
		NAME_None,				// Task 이름인데 별도로 지정X
		AbilityMontage,			// 재생할 몽타주
		1.f,					// 재생 속도 (1.f 이 기본속도)
		NAME_None
	);

	if (!MontageTask)
	{
		EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, true);
		return;
	}

	// 정상 완료시 호출할 함수 등록.
	MontageTask->OnCompleted.AddDynamic(this, &UCXGA_Fire::OnAttackCompleted);

	MontageTask->OnInterrupted.AddDynamic(this, &UCXGA_Fire::OnAttackInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UCXGA_Fire::OnAttackInterrupted);

	// 함수 연결 후 Task 실행
	MontageTask->ReadyForActivation();
}

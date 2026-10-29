// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Combat/CXGA_Dodge.h"

#include "CXVI/Character/Combat/CXCombatCharacterBase.h"

#include "Tags/CXGameplayTags.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGA_Dodge)

UCXGA_Dodge::UCXGA_Dodge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(CXVIGameplayTags::Ability_Dodge);

	SetAssetTags(Tags);
}

void UCXGA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{

	if (!Montage || !DodgeWindowEffect)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 설정한 비용과 쿨다운 적용.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Display, TEXT("%s: 스킬 사용 확정 실패. 비용 또는 쿨다운 확인 "), *GetName());

		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DodgeWindowEffect, GetAbilityLevel());

	if (!Spec.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	DodgeWindowHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);

	if (!DodgeWindowHandle.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("회피 판정 시작"));

	RegisterMontageTask();
}

bool UCXGA_Dodge::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	bool Result = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);

	return Result;
}

void UCXGA_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 회피가 일찍 종료되거나 취소되면 남은 판정도 제거
	if (DodgeWindowHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveActiveGameplayEffect(DodgeWindowHandle);
		}

		DodgeWindowHandle = FActiveGameplayEffectHandle();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	ACXCombatCharacterBase* Character = Cast<ACXCombatCharacterBase>(ActorInfo->AvatarActor.Get());

	if (!Character)
	{
		return;
	}

	Character->NotifyBattleAbilityEnded(Handle, bWasCancelled);
}

void UCXGA_Dodge::OnAbilityCompleted()
{
	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, false); // 마지막 인자 false -> 정상 종료됐다
}

void UCXGA_Dodge::OnAbilityInterrupted()
{
	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, true); // 마지막 인자 false -> 정상 종료됐다
}

void UCXGA_Dodge::RegisterMontageTask()
{
	// 몽타주를 재생할 Task
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,					// 사용할 어빌리티
		NAME_None,				// Task 이름인데 별도로 지정X
		Montage,		// 재생할 몽타주
		2.f,					// 재생 속도 (1.f 이 기본속도)
		NAME_None
	);

	if (!MontageTask)
	{
		EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, true);
		return;
	}

	// 정상 완료시 호출할 함수 등록.
	MontageTask->OnCompleted.AddDynamic(this, &UCXGA_Dodge::OnAbilityCompleted);

	MontageTask->OnInterrupted.AddDynamic(this, &UCXGA_Dodge::OnAbilityInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UCXGA_Dodge::OnAbilityInterrupted);

	// 함수 연결 후 Task 실행
	MontageTask->ReadyForActivation();
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Combat/CXGameplayAbilityCombatBase.h"
#include "CXVI/Character/Combat/CXCombatCharacterBase.h"

#include "CXVI/Tags/CXGameplayTags.h"

#include "CXVI/AbilitySystem/Abilities/Combat/CXAbilityData.h"
#include "AbilitySystem/Attributes/Combat/CXCombatAttributeSet.h"

#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGameplayAbilityCombatBase)

UCXGameplayAbilityCombatBase::UCXGameplayAbilityCombatBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UCXGameplayAbilityCombatBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 재생할 몽타주가 없으면 공격을 종료.
	if (!AbilityData || !AbilityData->AnimMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // 마지막 인자 true -> 취소 됐다
		return;
	}

	// 설정한 비용과 쿨다운 적용.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Display, TEXT("%s: 스킬 사용 확정 실패. 비용 또는 쿨다운 확인 "), *GetName());

		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ProcessedHitIndices.Reset();

	if (!RegisterHitEventTask())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	RegisterMontageTask();
}

bool UCXGameplayAbilityCombatBase::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	bool Result = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);

	return Result;
}

void UCXGameplayAbilityCombatBase::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);
}

void UCXGameplayAbilityCombatBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	ACXCombatCharacterBase* Character = Cast<ACXCombatCharacterBase>(ActorInfo->AvatarActor.Get());

	if (!Character)
	{
		return;
	}

	Character->NotifyBattleAbilityEnded(Handle, bWasCancelled);
}

float UCXGameplayAbilityCombatBase::GetManaCost() const
{
	return AbilityData ? FMath::Max(0.f, AbilityData->ManaCost) : 0.f;
}

bool UCXGameplayAbilityCombatBase::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 설정 누락을 비용 0인 스킬로 취급하지 않는다.
	if (!IsValid(AbilityData) || !CostGameplayEffectClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 스킬 데이터 또는 비용 GE가 설정되지 않았습니다."), *GetName());
		return false;
	}
	
	// 실제 자원 검사는 공통 비용 GE를 사용하는 GAS기본 구현에 맡긴다.
	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}

void UCXGameplayAbilityCombatBase::OnAttackCompleted()
{
	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, false); // 마지막 인자 false -> 정상 종료됐다
}

void UCXGameplayAbilityCombatBase::OnAttackInterrupted()
{
	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, true);
}

void UCXGameplayAbilityCombatBase::RegisterMontageTask()
{
	// 몽타주를 재생할 Task
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,					// 사용할 어빌리티
		NAME_None,				// Task 이름인데 별도로 지정X
		AbilityData->AnimMontage,		// 재생할 몽타주
		1.f,					// 재생 속도 (1.f 이 기본속도)
		NAME_None
	);

	if (!MontageTask)
	{
		EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, true);
		return;
	}

	// 정상 완료시 호출할 함수 등록.
	MontageTask->OnCompleted.AddDynamic(this, &UCXGameplayAbilityCombatBase::OnAttackCompleted);

	MontageTask->OnInterrupted.AddDynamic(this, &UCXGameplayAbilityCombatBase::OnAttackInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UCXGameplayAbilityCombatBase::OnAttackInterrupted);

	// 함수 연결 후 Task 실행
	MontageTask->ReadyForActivation();
}

void UCXGameplayAbilityCombatBase::OnAttackEvent(FGameplayEventData Payload)  
{
	ACXCombatCharacterBase* Attacker = Cast<ACXCombatCharacterBase>(GetAvatarActorFromActorInfo());

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

	if (!IsValid(Attacker) || !Attacker->HasAuthority() ||
		!IsValid(SourceASC) || !IsValid(AbilityData) || !SkillDamageEffect)
	{
		return;
	}

	// Notify 번호는 1부터 배열인덱스는 0부터
	const int32 HitNumber = static_cast<int32>(Payload.EventMagnitude);

	const int32 HitIndex = HitNumber - 1;

	if (!AbilityData->HitDamageMultipilers.IsValidIndex(HitIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: %d에 대응하는 데미지 배율이 없습니다."), *GetName(), HitNumber);
		return;
	}

	if (ProcessedHitIndices.Contains(HitIndex))
	{
		return;
	}
	ACXCombatCharacterBase* Target = Cast<ACXCombatCharacterBase>(Attacker->GetSelectedCombatTarget());

	if (!IsValid(Target) || !Target->GetIsCharacterAlive())
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	
	if (!IsValid(TargetASC))
	{
		return;
	}

	// 현재 공격력 : AttackRate

	const float AttackPower = FMath::Max(0.f, SourceASC->GetNumericAttribute(UCXCombatAttributeSet::GetAttackRateAttribute()));

	const float Multiplier = FMath::Max(0.f, AbilityData->HitDamageMultipilers[HitIndex]);

	const float Damage = AttackPower * Multiplier;
	if (Damage <= 0.f)
	{
		return;
	}

	// 타격마다 새로운 Spec을 만들어 이번데미지를 담는다.
	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(SkillDamageEffect, GetAbilityLevel());

	if (!Spec.IsValid())
	{
		return;
	}

	Spec.Data->SetSetByCallerMagnitude(CXVIGameplayTags::Data_Damage, Damage);

	// GE적용 중 콜백이 발생해도 중복 처리 되지 않도록 먼저 기록.
	ProcessedHitIndices.Add(HitIndex);

	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);

	UE_LOG(LogTemp, Display, TEXT("%s -> %s: %d타, 배율 %.2f, 요청 데미지: %.1f"), 
		*Attacker->GetName(),
		*Target->GetName(),
		HitNumber,
		Multiplier,
		Damage);
}

bool UCXGameplayAbilityCombatBase::RegisterHitEventTask()
{
	UAbilityTask_WaitGameplayEvent* Task = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		CXVIGameplayTags::Event_Combat_AttackHit,
		nullptr,
		false,	// 모든 타격이벤트를 받음
		true	// 정확히 일치하는 태그만 받음
		);

	if (!Task)
	{
		return false;
	}

	Task->EventReceived.AddDynamic(this, &UCXGameplayAbilityCombatBase::OnAttackEvent);

	Task->ReadyForActivation();

	return true;
}

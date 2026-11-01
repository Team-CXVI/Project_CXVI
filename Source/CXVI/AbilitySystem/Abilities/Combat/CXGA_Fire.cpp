// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/AbilitySystem/Abilities/Combat/CXGA_Fire.h"

#include "CXVI/Tags/CXGameplayTags.h"

#include "CXVI/Character/Combat/CXCombatCharacterPlayer.h"
#include "Character/Combat/CXCombatCharacterEnemy.h"
#include "Character/Combat/Components/CXWeakPointComponent.h"
#include "AbilitySystem/Attributes/Combat/CXCombatAttributeSetBase.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
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
	if (!ActorInfo || !CostGameplayEffectClass)
	{
		return false;
	}

	// 비용 GE적용후 AP가 음수가 되는지 등을 엔진에서 검사
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	ACXCombatCharacterPlayer* Character = Cast<ACXCombatCharacterPlayer>(ActorInfo->AvatarActor.Get());
	
	return IsValid(Character) && Character->GetIsCharacterAlive() && Character->GetIsTurn() && Character->GetIsAimMode();
}

void UCXGA_Fire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ACXCombatCharacterPlayer* Character = Cast<ACXCombatCharacterPlayer>(ActorInfo->AvatarActor.Get());

	if (!IsValid(Character) || !AbilityMontage || !AimDamageEffect)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // 마지막 인자 true -> 취소 됐다
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Display, TEXT("[자유조준] 발사 실패: 비용 또는 쿨다운 확인"));

		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("[자유조준] 발사 확정"));

	FHitResult Hit;
	
	
	if (Character->PerformAimTrace(Hit))
	{
		ApplyAimDamage(Hit);
	}

	RegisterMontageTask();
}

void UCXGA_Fire::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ACXCombatCharacterPlayer* Character = ActorInfo ? Cast<ACXCombatCharacterPlayer>(ActorInfo->AvatarActor.Get()) : nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	if (IsValid(Character))
	{
		Character->NotifyBattleAbilityEnded(Handle, bWasCancelled);
	}
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

void UCXGA_Fire::ApplyAimDamage(const FHitResult& Hit)
{
	ACXCombatCharacterPlayer* Attacker = Cast<ACXCombatCharacterPlayer>(GetAvatarActorFromActorInfo());

	ACXCombatCharacterEnemy* Target = Cast<ACXCombatCharacterEnemy>(Hit.GetActor());

	if (!IsValid(Attacker) || !Attacker->HasAuthority() ||
		!IsValid(Target) || !Target->GetIsCharacterAlive() || !AimDamageEffect)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();

	if (!IsValid(SourceASC) || !IsValid(TargetASC))
	{
		return;
	}

	// 맞힌 컴포넌트가 아직 파괴되지 않은 약점인지 확인
	UCXWeakPointComponent* WeakPoint = Cast<UCXWeakPointComponent>(Hit.GetComponent());

	const bool bWeakPointHit = IsValid(WeakPoint) && WeakPoint->GetOwner() == Target && !WeakPoint->IsBroken();

	const float WeakPointMultiplier = bWeakPointHit ? WeakPoint->GetDamageMultiplier() : 1.f;

	const float AttackPower = FMath::Max(0.f, SourceASC->GetNumericAttribute(UCXCombatAttributeSetBase::GetAttackPowerAttribute()));

	const float Damage = AttackPower * FMath::Max(0.f, AimDamageMultiplier) * WeakPointMultiplier;

	if (Damage <= 0.f)
	{
		return;
	}

	// 공격자 정보와 레이 명중 정보를 피해 GE에 전달
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddHitResult(Hit);

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(AimDamageEffect, GetAbilityLevel(), Context);

	if (!Spec.IsValid())
	{
		return;
	}

	Spec.Data->SetSetByCallerMagnitude(CXVIGameplayTags::Data_Damage, Damage);
	Spec.Data->AddDynamicAssetTag(CXVIGameplayTags::Damage_CannotBeDodged);

	const float HealthBefore = TargetASC->GetNumericAttribute(UCXCombatAttributeSetBase::GetHealthAttribute());

	UE_LOG(LogTemp, Display,
		TEXT("[자유조준] %s -> %s, 약점배율 %.1f, 요청 피해 %.1f"),
		*Attacker->GetName(),
		*Target->GetName(),
		WeakPointMultiplier,
		Damage);

	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);

	if (!bWeakPointHit || !IsValid(WeakPoint) || !IsValid(TargetASC))
	{
		return;
	}

	const float HealthAfter = TargetASC->GetNumericAttribute(UCXCombatAttributeSetBase::GetHealthAttribute());

	if (HealthAfter < HealthBefore)
	{
		WeakPoint->TryBreak();
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("[약점] 실제 체력 감소가 없어 약점 유지"));
	}
}

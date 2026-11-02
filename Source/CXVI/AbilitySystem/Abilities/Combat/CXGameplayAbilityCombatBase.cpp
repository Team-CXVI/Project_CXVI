// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Combat/CXGameplayAbilityCombatBase.h"
#include "CXVI/Character/Combat/CXCombatCharacterBase.h"
#include "Character/Combat/Components/CXJustTimingComponent.h"

#include "CXVI/Tags/CXGameplayTags.h"

#include "CXVI/AbilitySystem/Abilities/Combat/CXAbilityData.h"
#include "AbilitySystem/Attributes/Combat/CXCombatAttributeSetBase.h"
#include "AbilitySystem/Attributes/Combat/CXCombatPlayerAttributeSet.h"

#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGameplayAbilityCombatBase)

UCXGameplayAbilityCombatBase::UCXGameplayAbilityCombatBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

float UCXGameplayAbilityCombatBase::GetAPCost() const
{
	return AbilityData ? AbilityData->APCost : 0.f;
}

void UCXGameplayAbilityCombatBase::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!IsValid(AbilityData) || !ActorInfo)
	{
		return;
	}

	const UAbilitySystemComponent* OwnerASC = ActorInfo->AbilitySystemComponent.Get();

	if (!IsValid(OwnerASC))
	{
		return;
	}

	if (!OwnerASC->GetSet<UCXCombatPlayerAttributeSet>())
	{
		return;
	}

	const float Cost = GetAPCost();

	if (Cost == 0.f || !CostGameplayEffectClass)
	{
		return;
	}

	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CostGameplayEffectClass, GetAbilityLevel(Handle, ActorInfo));

	if (!Spec.IsValid())
	{
		return;
	}

	Spec.Data->SetSetByCallerMagnitude(CXVIGameplayTags::Data_AP, -Cost);

	// 이 함수가 비용 적용을 담당하므로 Super::ApplyCost는 호출하지 않음.
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
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
	ProcessedComboStages.Reset();
	SuccessfulComboSections.Reset();

	const bool bHitTaskReady = RegisterHitEventTask();

	const bool bComboTaskReady = AbilityData->JustTimingType != ECXJustTimingType::ContinueAttack || RegisterComboEventTask();

	if (!bHitTaskReady || !bComboTaskReady)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACXCombatCharacterBase* Character = Cast<ACXCombatCharacterBase>(GetAvatarActorFromActorInfo());
	if (!IsValid(Character))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (UCXJustTimingComponent* Timing = Character->FindComponentByClass<UCXJustTimingComponent>())
	{
		Timing->ResetTiming();
	}

	Character->NotifyAttackStarted(AbilityData->HitDamageMultipilers.Num());


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

	if (UCXJustTimingComponent* Timing = Character->FindComponentByClass<UCXJustTimingComponent>())
	{
		Timing->ResetTiming();
	}

	Character->NotifyBattleAbilityEnded(Handle, bWasCancelled);
}

bool UCXGameplayAbilityCombatBase::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 설정 누락을 비용 0인 스킬로 취급하지 않는다.
	if (!IsValid(AbilityData) || !ActorInfo)
	{
		return false;
	}
	
	UAbilitySystemComponent* OwnerASC = ActorInfo->AbilitySystemComponent.Get();

	if (!IsValid(OwnerASC))
	{
		return false;
	}

	const UCXCombatPlayerAttributeSet* PlayerAttributes = OwnerASC->GetSet<UCXCombatPlayerAttributeSet>();

	if (!IsValid(PlayerAttributes))
	{
		return true;
	}

	const float Cost = GetAPCost();

	// 무료 행동과 AP를 획득하는 행동은 잔량 검사가 필요없다.
	if (Cost == 0.f)
	{
		return true;
	}

	if (!CostGameplayEffectClass)
	{
		return false;
	}

	return Cost < 0.f || PlayerAttributes->GetAP() >= Cost;
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

	// 같은 타격 이벤트는 한 번만 처리
	ProcessedHitIndices.Add(HitIndex);

	bool bJustTimingSuccess = false;

	// 데미지 증가형과 공격 연계형 모두 판정 결과를 확인.
	if (AbilityData->JustTimingType == ECXJustTimingType::DamageBoost)
	{
		if (UCXJustTimingComponent* Timing = Attacker->FindComponentByClass<UCXJustTimingComponent>())
		{
			const int32 RequiredCount = AbilityData->RequiredTimingCounts.IsValidIndex(HitIndex) ?
				FMath::Max(1, AbilityData->RequiredTimingCounts[HitIndex]) : 1;

			bJustTimingSuccess = Timing->ConsumeSuccess(HitNumber, RequiredCount);
		}
	}

	// 판정 결과로 다음 공격을 연결.
	// 대상이 사망했거나 데미지가 0이여도 연계판단은 수행.
	//UpdateNextAttackSection(HitIndex, bJustTimingSuccess);

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

	const float AttackPower = FMath::Max(0.f, SourceASC->GetNumericAttribute(UCXCombatAttributeSetBase::GetAttackPowerAttribute()));

	const float BaseMultiplier = FMath::Max(0.f, AbilityData->HitDamageMultipilers[HitIndex]);

	/*bool bJustTimingSuccess = false;

	if (AbilityData->JustTimingType == ECXJustTimingType::DamageBoost)
	{
		if (UCXJustTimingComponent* Timing = Attacker->FindComponentByClass<UCXJustTimingComponent>())
		{
			const int32 RequiredCount = AbilityData->RequiredTimingCounts.IsValidIndex(HitIndex)
				? FMath::Max(1, AbilityData->RequiredTimingCounts[HitIndex]) : 1;

			bJustTimingSuccess = Timing->ConsumeSuccess(HitNumber, RequiredCount);

		}
	}*/
	// 데미지 증가형에서 성공했을때만 증가.
	// 공격 연계형은 판정에 성공해도 데미지 배율 1 유지.
	const bool bApplyDamageBoost = AbilityData->JustTimingType == ECXJustTimingType::DamageBoost && bJustTimingSuccess;

	const float JustMultiplier = bApplyDamageBoost ? FMath::Max(1.f, AbilityData->JustTimingDamageMultiplier) : 1.f;

	const float Damage = AttackPower * BaseMultiplier * JustMultiplier;
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

	// 회피 불가 적용

	// 연계 성공으로 열린 동작인지 확인.
	// 같은 동작에 여러 타격이 있어도 모두 같은 결과 사용
	const bool bComboSectionSuccess = AbilityData->JustTimingType == ECXJustTimingType::ContinueAttack &&
		SuccessfulComboSections.Contains(SourceASC->GetCurrentMontageSectionName());

	const bool bCannotBeDodged = AbilityData->bJustTimingCannotBeDodged &&
		(bJustTimingSuccess || bComboSectionSuccess);

	if (bCannotBeDodged)
	{
		Spec.Data->AddDynamicAssetTag(CXVIGameplayTags::Damage_CannotBeDodged);
	}


	// GE적용 중 콜백이 발생해도 중복 처리 되지 않도록 먼저 기록.
	//ProcessedHitIndices.Add(HitIndex);

	UE_LOG(LogTemp, Display, TEXT("[공격] %s -> %s: %d타, 기본 배율 %.2f, 저스트 %s, 저스트 배율 %.2f, 요청 데미지: %.1f"), 
		*Attacker->GetName(),
		*Target->GetName(),
		HitNumber,
		BaseMultiplier,
		bJustTimingSuccess ? TEXT("성공") : TEXT("실패"),
		JustMultiplier,
		Damage);

	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);


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

void UCXGameplayAbilityCombatBase::UpdateNextAttackSection(int32 HitIndex, bool bSuccess)
{
	if (!IsValid(AbilityData) || AbilityData->JustTimingType != ECXJustTimingType::ContinueAttack)
	{
		return;
	}

	UAnimMontage* Montage = AbilityData->AnimMontage;

	if (!IsValid(Montage) || !AbilityData->AttackSectionNames.IsValidIndex(HitIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[연계] %d타의 공격 섹션 설정이 없습니다."), HitIndex + 1);
		return;
	}

	const FName CurrentSection = AbilityData->AttackSectionNames[HitIndex];

	// 기본적으로 마무리 섹션으로 진행.
	FName NextSection = AbilityData->EndSectionName;

	// 성공했고 다음 공격이 있으면 해당 공격으로 이동
	if (bSuccess && AbilityData->AttackSectionNames.IsValidIndex(HitIndex + 1))
	{
		NextSection = AbilityData->AttackSectionNames[HitIndex + 1];
	}

	const bool bInvalidCurrentSection = Montage->GetSectionIndex(CurrentSection) == INDEX_NONE;

	// None은 섹션 이름이 아니라 '다음 섹션 없이 종료' 라는 의미
	const bool bInvalidNextSection = !NextSection.IsNone() && Montage->GetSectionIndex(NextSection) == INDEX_NONE;

	if (bInvalidCurrentSection || bInvalidNextSection)
	{
		UE_LOG(LogTemp, Warning, TEXT("[연계] 몽타주 섹션을 찾을수 없습니다: %s -> %s"), *CurrentSection.ToString(), *NextSection.ToString());
		return;
	}

	/*if (Montage->GetSectionIndex(CurrentSection) == INDEX_NONE || Montage->GetSectionIndex(NextSection) == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[연계] 몽타주 섹션을 찾을수 없습니다: %s -> %s"), *CurrentSection.ToString(), *NextSection.ToString());
		return;
	}*/

	// 현재 섹션은 끝까지 재생하고 다음 섹션을 변경.
	MontageSetNextSectionName(CurrentSection, NextSection);
	
	UE_LOG(LogTemp, Display, TEXT("[연계] %d단계 판정 %s: %s → %s"), 
		HitIndex + 1,
		bSuccess ? TEXT("성공") : TEXT("실패"),
		*CurrentSection.ToString(),
		*NextSection.ToString());
}

bool UCXGameplayAbilityCombatBase::RegisterComboEventTask()
{
	UAbilityTask_WaitGameplayEvent* Task = 
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, 
			CXVIGameplayTags::Event_Combat_ComboBranch,
			nullptr,
			false,
			true
		);
	
	if(!Task)
	{ 
		return false;
	}

	Task->EventReceived.AddDynamic(this, &UCXGameplayAbilityCombatBase::OnComboBranchEvent);

	Task->ReadyForActivation();
	return true;
}

void UCXGameplayAbilityCombatBase::OnComboBranchEvent(FGameplayEventData Payload)
{
	AActor* Owner = GetAvatarActorFromActorInfo();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (!IsValid(Owner) || !Owner->HasAuthority() ||
		!IsValid(ASC) || !IsValid(AbilityData) ||
		!IsValid(AbilityData->AnimMontage) ||
		AbilityData->JustTimingType != ECXJustTimingType::ContinueAttack)
	{
		return;
	}

	const int32 StageNumber = static_cast<int32>(Payload.EventMagnitude);

	const int32 StageIndex = StageNumber - 1;

	// 마지막 동작에는 다른 동작을 여는 노티파이가 필요 없음.
	if (!AbilityData->AttackSectionNames.IsValidIndex(StageIndex) ||
		!AbilityData->AttackSectionNames.IsValidIndex(StageIndex + 1))
	{
		return;
	}

	const FName CurrentSection = AbilityData->AttackSectionNames[StageIndex];

	const FName NextSection = AbilityData->AttackSectionNames[StageIndex + 1];

	const FName ActualSection = ASC->GetCurrentMontageSectionName();

	const int32 NextSectionIndex = AbilityData->AnimMontage->GetSectionIndex(NextSection);

	// 다른 동작에서 잘못 전달된 이벤트는 무시.
	if (ActualSection != CurrentSection || NextSection.IsNone() || NextSectionIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[연계] %d단계: 실제 섹션=%s, 설정 섹션=%s, 다음 섹션=%s, 다음 인덱스=%d"),
			StageNumber,
			*ActualSection.ToString(),
			*CurrentSection.ToString(),
			*NextSection.ToString(),
			NextSectionIndex);

		return;
	}

	ProcessedComboStages.Add(StageIndex);

	bool bSuccess = false;

	if (UCXJustTimingComponent* Timing = Owner->FindComponentByClass<UCXJustTimingComponent>())
	{
		const int32 RequiredCount = AbilityData->RequiredTimingCounts.IsValidIndex(StageIndex)
			? FMath::Max(1, AbilityData->RequiredTimingCounts[StageIndex]) : 1;

		// 연계형에서는 동작 번호를 판정 그룹 번호로 사용
		bSuccess = Timing->ConsumeSuccess(StageNumber, RequiredCount);
	}

	// 기존 함수에 데미지 타격번호대신 공격 동작 인덱스 전달.
	UpdateNextAttackSection(StageIndex, bSuccess);

	if (bSuccess)
	{
		SuccessfulComboSections.Add(NextSection);
	}

	UE_LOG(LogTemp, Display, TEXT("[연계 판정] %s: %s → %s"),
		*CurrentSection.ToString(),
		bSuccess ? TEXT("성공") : TEXT("실패"),
		bSuccess ? *NextSection.ToString() : *AbilityData->EndSectionName.ToString());
}

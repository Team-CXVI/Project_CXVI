// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/AbilitySystem/Abilities/Combat/CXGA_AttackBase.h"

#include "CXVI/Tags/CXGameplayTags.h"

#include "CXVI/AbilitySystem/Abilities/Combat/CXCharacterAbilityData.h"

#include "CXVI/Character/Combat/CXCombatCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGA_AttackBase)

UCXGA_AttackBase::UCXGA_AttackBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(CXVIGameplayTags::Ability_Basic_Attack);

	SetAssetTags(Tags);
}

void UCXGA_AttackBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 재생할 몽타주가 없으면 공격을 종료.
	if (!BasicAttackMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // 마지막 인자 true -> 취소 됐다
		return;
	}

	// 설정한 비용과 쿨다운 적용.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 이벤트 태스크 등록
	RegisterEventTask();

	// 몽타주 태스크 등록
	RegisterMontageTask();

	// 콤보 시작
	ProcessComboCommand();
}

bool UCXGA_AttackBase::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	bool Result = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);

	return Result;
}

void UCXGA_AttackBase::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	ProcessComboCommand();
}

void UCXGA_AttackBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	CurrentCombo = 0;

	ACXCombatCharacterBase* Character = Cast<ACXCombatCharacterBase>(ActorInfo->AvatarActor.Get());
	if (Character)
	{
		Character->NotifyBattleAbilityEnded(Handle, bWasCancelled);
	}
}

void UCXGA_AttackBase::OnAttackCompleted()
{
	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, false); // 마지막 인자 false -> 정상 종료됐다
}

void UCXGA_AttackBase::OnAttackInterrupted()
{
	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, true);
}

void UCXGA_AttackBase::OnAttackEvent(FGameplayEventData Payload)
{
}

void UCXGA_AttackBase::RegisterEventTask()
{
}

void UCXGA_AttackBase::RegisterMontageTask()
{
	// 몽타주를 재생할 Task
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,					// 사용할 어빌리티
		NAME_None,				// Task 이름인데 별도로 지정X
		BasicAttackMontage,		// 재생할 몽타주
		1.f,					// 재생 속도 (1.f 이 기본속도)
		NAME_None
	);

	if (!MontageTask)
	{
		EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, true);
		return;
	}

	// 정상 완료시 호출할 함수 등록.
	MontageTask->OnCompleted.AddDynamic(this, &UCXGA_AttackBase::OnAttackCompleted);

	MontageTask->OnInterrupted.AddDynamic(this, &UCXGA_AttackBase::OnAttackInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UCXGA_AttackBase::OnAttackInterrupted);

	// 함수 연결 후 Task 실행
	MontageTask->ReadyForActivation();
}

void UCXGA_AttackBase::ProcessComboCommand()
{
	if (CurrentCombo == 0)
	{
		ComboActionBegin();
		return;
	}

	// 타이머 핸들의 유효성 여부로 다음 공격으로 분기 결정
	if (ComboTimerHandle.IsValid())
	{
		bHasNextComboCommand = true;
	}
	else
	{
		bHasNextComboCommand = false;
	}
}

void UCXGA_AttackBase::ComboActionBegin()
{
	CurrentCombo = 1;

	ComboTimerHandle.Invalidate();

	SetComboCheckTimer();
}

void UCXGA_AttackBase::ComboActionEnd()
{
}

void UCXGA_AttackBase::SetComboCheckTimer()
{
	// 현재 콤보를 실제 배열의 인덱스로
	const int32 ComboIndex = CurrentCombo - 1;

	// 콤보인덱스가 유효한지
	ensureAlways(ComboActionData->EffectiveFrameCount.IsValidIndex(ComboIndex));

	// 애니메이션 재생속도
	const float AttackSpeedRate = 1.f;

	// 콤보 공격 입력 시간 초단위 계산
	float ComboEffectTime = (ComboActionData->EffectiveFrameCount[ComboIndex] / ComboActionData->FrameRate) / AttackSpeedRate;

	// 타이머 설정
	if (ComboEffectTime > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(ComboTimerHandle, this, &UCXGA_AttackBase::ComboCheck, ComboEffectTime);
	}
}

void UCXGA_AttackBase::ComboCheck()
{
	// 타이머 재사용을 위한 초기화
	ComboTimerHandle.Invalidate();

	if (bHasNextComboCommand)
	{
		CurrentCombo = FMath::Clamp(CurrentCombo + 1, 1, ComboActionData->MaxComboCount);
		UE_LOG(LogTemp, Display, TEXT("Current Combo : %d"), CurrentCombo);

		FName NextSection = *FString::Printf(TEXT("%s%d"), *ComboActionData->MontageSectionNamePrefix, CurrentCombo);

		MontageJumpToSection(NextSection);

		SetComboCheckTimer();

		bHasNextComboCommand = false;
	}
}

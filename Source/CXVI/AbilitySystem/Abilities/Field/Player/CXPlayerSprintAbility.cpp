// Fill out your copyright notice in the Description page of Project Settings.


#include "CXPlayerSprintAbility.h"
#include "CXVI/Tags/CXFieldTags.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitAttributeChange.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXPlayerSprintAbility)

UCXPlayerSprintAbility::UCXPlayerSprintAbility()
{
	// 캐릭터별로 속도와 이펙트 핸들을 보관할 인스턴스 생성.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Ability 실행 중 ASC에 붙고, 종료동료되면 제거되는 태그.
	ActivationOwnedTags.AddTag(CXVIFieldTag::Input_Field_Sprint);
	

	// 이동이 차단된 상태에서는 시작 금지.
	ActivationBlockedTags.AddTag(CXVIFieldTag::State_MovingBlock);

	// 기존에 만든 스테미나 소모 이펙트를 기본값으로 연결.
	//static ConstructorHelpers::FClassFinder<UGameplayEffect> DrainEffectClass(
	//	TEXT("/Game/ThirdPerson/GameEffects/Effects/MoveEffects/BPGE_SprintStamina"));

	//if (DrainEffectClass.Succeeded())
	//{
	//	StaminaDrainEffect = DrainEffectClass.Class;
	//}
}

bool UCXPlayerSprintAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (!ActorInfo)
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());


	return Character && Character->GetCharacterMovement();
}

void UCXPlayerSprintAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;

	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;

	if (!IsValid(ASC) || !Movement)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 기존 속도를 저장하고 스프린트 속도로 변경.
	OriginalWalkSpeed = Movement->MaxWalkSpeed;
	SprintMovement = Movement;
	Movement->MaxWalkSpeed = SprintSpeed;
	

	OnSprintStarted();
}

void UCXPlayerSprintAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo))
	{
		return;
	}

	// GAS 내부 잠금 중에는 정리까지 함께 지연.
	if (ScopeLockCount > 0)
	{
		WaitingToExecute.Add(FPostLockDelegate::CreateUObject(
			this,
			&UCXPlayerSprintAbility::EndAbility,
			Handle,
			ActorInfo,
			ActivationInfo,
			bReplicateEndAbility,
			bWasCancelled
		));

		return;
	}


	// 원래 이동 속도로 복구.
	if (UCharacterMovementComponent* Movement = SprintMovement.Get())
	{
		Movement->MaxWalkSpeed = OriginalWalkSpeed;
	}

	SprintMovement.Reset();

	// GAS가 활성 상태와 소유 태그를 정리.
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


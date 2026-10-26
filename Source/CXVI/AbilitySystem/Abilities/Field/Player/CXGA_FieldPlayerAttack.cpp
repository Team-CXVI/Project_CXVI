// Fill out your copyright notice in the Description page of Project Settings.


#include "CXGA_FieldPlayerAttack.h"
#include "Tags/CXFieldTags.h"
#include "Character/Field/CXFieldPlayerCharacter.h"
#include "Character/Field/CXFieldEnemy.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGA_FieldPlayerAttack)

UCXGA_FieldPlayerAttack::UCXGA_FieldPlayerAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	ActivationOwnedTags.AddTag(CXVIFieldTag::Input_Field_Attack);
	ActivationOwnedTags.AddTag(CXVIFieldTag::State_MovingBlock);

	
}

bool UCXGA_FieldPlayerAttack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(ActorInfo->AvatarActor.Get());


	return IsValid(ASC) && IsValid(Player);
}

void UCXGA_FieldPlayerAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(ActorInfo->AvatarActor.Get());

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	UAbilityTask_WaitGameplayEvent* AttackHitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		CXVIFieldTag::Event_Field_AttackHit, 
		nullptr, // 이 GA의 ASC에서 수신.
		true,	 // 이번 활성화에서는 한 번만 수신
		true);	 // 정확히 같은 태그만 수신.

	AttackHitTask->EventReceived.AddDynamic(this, &ThisClass::OnAttackHitEvent);
	AttackHitTask->ReadyForActivation();

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AttackMontage, 2.0f, NAME_None, true);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnAttackInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnAttackInterrupted);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnAttackCompleted);
	MontageTask->ReadyForActivation();

	if (!IsActive())
	{
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("어택어빌리티 성공"));
}

void UCXGA_FieldPlayerAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo))
	{
		return;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UCXGA_FieldPlayerAttack::OnAttackHitEvent(FGameplayEventData Payload)
{
	if (!IsActive())
	{
		return;
	}
	
	AActor* Player = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();

	if (!IsValid(Player) || !World)
	{
		return;
	}

	const float Range = FMath::Max(2.0f, AttackRange);
	const float Radius = FMath::Clamp(AttackRadius, 1.0f, Range * 0.5f);

	const FVector Forward = Player->GetActorForwardVector();
	const FVector Origin = Player->GetActorLocation();

	// 구체 중심이 아닌 판정 전체가 전방 0~Range에 들어가도록 배치.
	const FVector Start = Origin + Forward * Radius;
	const FVector End = Origin + Forward * (Range - Radius);

	// 자기 자신은 검사에서 제외.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FieldAttack), false, Player);

	FHitResult Hit;

	const bool bHit = World->SweepSingleByChannel(
		Hit,
		Start,
		End, 
		FQuat::Identity, 
		ECC_Visibility,
		FCollisionShape::MakeSphere(Radius),
		QueryParams);

	ACXFieldEnemy* Enemy = bHit ? Cast<ACXFieldEnemy>(Hit.GetActor()) : nullptr;
	
	// 드로우 디버그 모드이면.
	if (bDrawAttackDebug)
	{
		const FVector Center = (Start + End) * 0.5f;
		const float HalfHeight =
			FVector::Distance(Start, End) * 0.5f + Radius;

		DrawDebugCapsule(
			World,
			Center,
			HalfHeight,
			Radius,
			FRotationMatrix::MakeFromZ(Forward).ToQuat(),
			IsValid(Enemy) ? FColor::Green : FColor::Red,
			false,
			1.0f);
	}

	if (!IsValid(Enemy))
	{
		UE_LOG(LogTemp, Log, TEXT("[FieldAttack] 적 명중 없음. Blocker:%s"), *GetNameSafe(Hit.GetActor()));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[FieldAttack] 명중 Enemy=%s Group=%s Spawn=%s"),*Enemy->GetName(),*Enemy->GetEnemyGroupId().ToString(), *Enemy->GetSpawnId().ToString());
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Combat/CXGameplayAbilityCombatBase.h"
#include "CXVI/Character/Combat/CXCombatCharacterBase.h"
#include "Character/Combat/CXCombatCharacterPlayer.h"
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
#include "NiagaraSystem.h"
#include "Components/CapsuleComponent.h"      // 캡슐 반높이로 발밑 위치 계산
#include "Components/SkeletalMeshComponent.h" // 가슴·머리 소켓의 월드 위치 조회

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGameplayAbilityCombatBase)

UCXGameplayAbilityCombatBase::UCXGameplayAbilityCombatBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

float UCXGameplayAbilityCombatBase::GetAPCost() const
{
	return AbilityData ? AbilityData->APCost : 0.f;
}

bool UCXGameplayAbilityCombatBase::ShouldApproachTarget() const
{
	return AbilityData ? AbilityData->bShouldApproach : true;
}

ECXAttackTargetMode UCXGameplayAbilityCombatBase::GetAttackTargetMode() const
{
	return AbilityData ? AbilityData->AttackTargetMode : ECXAttackTargetMode::Single;
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

	const float Cost = GetAPCostForSpec(Handle, ActorInfo);

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

	Character->NotifyAttackStarted(AbilityData->HitData.Num());


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

	const float Cost = GetAPCostForSpec(Handle, ActorInfo);

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
		NAME_None,
		true,
		1.f,
		0.f,
		true
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

	if (!AbilityData->HitData.IsValidIndex(HitIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: %d에 대응하는 데미지 배율이 없습니다."), *GetName(), HitNumber);
		return;
	}

	const FCXAttackHitData& CurrentHit = AbilityData->HitData[HitIndex];

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
				FMath::Max(0, AbilityData->RequiredTimingCounts[HitIndex]) : 1;

			if (RequiredCount > 0)
			{
				bJustTimingSuccess = Timing->ConsumeSuccess(HitNumber, RequiredCount);
			}
		}
	}

	// 판정 결과로 다음 공격을 연결.
	// 대상이 사망했거나 데미지가 0이여도 연계판단은 수행.
	//UpdateNextAttackSection(HitIndex, bJustTimingSuccess);

	const float AttackPower = FMath::Max(
		0.f,
		SourceASC->GetNumericAttribute(
			UCXCombatAttributeSetBase::GetAttackPowerAttribute()));

	const float BaseMultiplier =
		FMath::Max(0.f, CurrentHit.DamageMultiplier);

	const bool bApplyDamageBoost =
		AbilityData->JustTimingType == ECXJustTimingType::DamageBoost &&
		bJustTimingSuccess;

	const float JustMultiplier = bApplyDamageBoost
		? FMath::Max(1.f, AbilityData->JustTimingDamageMultiplier)
		: 1.f;

	const float Damage = AttackPower * BaseMultiplier * JustMultiplier;

	if (Damage <= 0.f)
	{
		return;
	}

	const bool bComboSectionSuccess = AbilityData->JustTimingType == ECXJustTimingType::ContinueAttack &&
		SuccessfulComboSections.Contains(SourceASC->GetCurrentMontageSectionName());

	const bool bCannotBeDodged = AbilityData->bAlwaysUndodgeable ||
		(AbilityData->bJustTimingCannotBeDodged &&
			(bJustTimingSuccess || bComboSectionSuccess));

	// 효과 적용 중 다른 콜백이 발생할 수 있으므로 목록을 복사.
	TArray<TWeakObjectPtr<ACXCombatCharacterBase>> Targets = Attacker->GetSelectedCombatTargets();

	// 플레이어의 기존 단일 대상 공격도 처리.
	if (Targets.IsEmpty())
	{
		ACXCombatCharacterBase* SingleTarget = Cast<ACXCombatCharacterBase>(Attacker->GetSelectedCombatTarget());

		if (IsValid(SingleTarget))
		{
			Targets.Add(SingleTarget);
		}
	}

	for (const auto& WeakTarget : Targets)
	{
		// 이전 대상의 효과 처리 중 어빌리티가 취소됐다면 중단.
		if (!IsActive() || !IsValid(SourceASC))
		{
			break;
		}

		ACXCombatCharacterBase* Target = WeakTarget.Get();

		if (!IsValid(Target) || !Target->GetIsCharacterAlive())
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

		if (!IsValid(TargetASC))
		{
			continue;
		}

		// 대상마다 별도의 Spec으로 피해 적용.
		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(SkillDamageEffect, GetAbilityLevel());

		if (!Spec.IsValid())
		{
			continue;
		}

		Spec.Data->SetSetByCallerMagnitude(CXVIGameplayTags::Data_Damage, Damage);

		if (bCannotBeDodged)
		{
			Spec.Data->AddDynamicAssetTag(CXVIGameplayTags::Damage_CannotBeDodged);
		}

		if (AbilityData->AttackTargetMode == ECXAttackTargetMode::AllOpponents)
		{
			Spec.Data->AddDynamicAssetTag(CXVIGameplayTags::Damage_CannotBeParried);
		}

		

		UObject* HitFXAsset = CurrentHit.HitCascadeFX.Get();
		if (!IsValid(HitFXAsset))
		{

			HitFXAsset = CurrentHit.HitFX.Get();
		}

		if (IsValid(HitFXAsset) && IsValid(Target) && IsValid(TargetASC))
		{
			FGameplayCueParameters Parameters;
			Parameters.SourceObject = HitFXAsset;
			
			FVector FXLocation = Target->GetActorLocation();

			if (CurrentHit.HitFXLocation == ECXHitFXLocation::Ground)
			{
				// 캡슐 중심에서 반높이만큼 내려 발밑 좌표를 구한다.
				const UCapsuleComponent* TargetCapsule = Target->GetCapsuleComponent();
				FXLocation = TargetCapsule->GetComponentLocation();
				FXLocation.Z -= TargetCapsule->GetScaledCapsuleHalfHeight();
			}
			else if (const USkeletalMeshComponent* TargetMesh = Target->GetMesh();
				TargetMesh && !CurrentHit.HitFXSocket.IsNone() &&
				TargetMesh->DoesSocketExist(CurrentHit.HitFXSocket))
			{
				FXLocation = TargetMesh->GetSocketLocation(CurrentHit.HitFXSocket);
			}

			Parameters.Location =
				FXLocation + Target->GetActorQuat().RotateVector(CurrentHit.HitFXOffset);
			Parameters.RawMagnitude = CurrentHit.HitFXScale;
			Parameters.Instigator = Attacker;
			Parameters.EffectCauser = Attacker;

			UE_LOG(LogTemp, Display, TEXT("[HitFX] Asset=%s Class=%s"),
				*GetPathNameSafe(HitFXAsset),
				*GetNameSafe(HitFXAsset->GetClass()));

			TargetASC->ExecuteGameplayCue(
				CXVIGameplayTags::GameplayCue_Combat_Hit,
				Parameters
			);
		}

		UE_LOG(LogTemp, Display,
			TEXT("[공격] %s -> %s: %d타, 기본 배율 %.2f, "
				"저스트 %s, 저스트 배율 %.2f, 요청 데미지: %.1f"),
			*Attacker->GetName(),
			*Target->GetName(),
			HitNumber,
			BaseMultiplier,
			bJustTimingSuccess ? TEXT("성공") : TEXT("실패"),
			JustMultiplier,
			Damage);

		SourceASC->ApplyGameplayEffectSpecToTarget(
			*Spec.Data.Get(),
			TargetASC);
	}
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

	const bool bJumpToNextAttack = bSuccess && AbilityData->AttackSectionNames.IsValidIndex(HitIndex + 1);

	if (bJumpToNextAttack)
	{
		// 현재 동작의 남은 부분을 생략하고 다음 공격 시작.
		MontageJumpToSection(NextSection);
	}
	else
	{
		// 실패하면 현재 동작을 끝까지 재생하고 종료.
		MontageSetNextSectionName(CurrentSection, NextSection);
	}
	
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

	const int32 RequiredCount = AbilityData->RequiredTimingCounts.IsValidIndex(StageIndex)
		? FMath::Max(0, AbilityData->RequiredTimingCounts[StageIndex]) : 1;

	if (RequiredCount == 0)
	{
		UpdateNextAttackSection(StageIndex, true);
		UE_LOG(LogTemp, Display, TEXT("[연계 판정] %s: 자동 진행 → %s"),
			*CurrentSection.ToString(), *NextSection.ToString());
		return;
	}

	bool bSuccess = false;

	if (UCXJustTimingComponent* Timing = Owner->FindComponentByClass<UCXJustTimingComponent>())
	{
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

float UCXGameplayAbilityCombatBase::GetAPCostForSpec(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	const auto* Player = ActorInfo ? Cast<ACXCombatCharacterPlayer>(ActorInfo->AvatarActor.Get()) : nullptr;

	if (Player)
	{
		if (const float* Cost = Player->FindPreparedSkillAPCost(Handle))
		{
			return *Cost;
		}
	}

	return GetAPCost();
}

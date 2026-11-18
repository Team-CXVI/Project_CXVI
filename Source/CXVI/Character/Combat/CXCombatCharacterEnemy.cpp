// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Character/Combat/CXCombatCharacterEnemy.h"

#include "CXVI/AbilitySystem/Attributes/Combat/CXCombatEnemyAttributeSet.h"
#include "Foundation/Combat/CombatMessage/CXCombatMessageData.h"
#include "Foundation/Combat/CXCombatManagerSubsystem.h"
#include "AbilitySystem/Abilities/Combat/CXAbilityData.h"
#include "AbilitySystem/Abilities/Combat/CXGameplayAbilityCombatBase.h"

#include "Tags/CXGameplayTags.h"

#include "Foundation/Combat/CXCollisionChannels.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h" 
#include "Animation/AnimMontage.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatCharacterEnemy)


ACXCombatCharacterEnemy::ACXCombatCharacterEnemy()
{
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -88.f), FRotator(0.f, -90.f, 0.f));
	// 라이팅 채널 : 0(기본 라이트) + 2(몬스터 전용 라이트, 아레나의 DL_*_EnemyKey). 아군은 부모의 0 + 1 사용
	GetMesh()->SetLightingChannels(true, false, true);

	

	AttributeSet = CreateDefaultSubobject<UCXCombatEnemyAttributeSet>(TEXT("AttributeSet"));
	ASC->AddAttributeSetSubobject<UCXCombatEnemyAttributeSet>(AttributeSet);


}

void ACXCombatCharacterEnemy::PostInitializeComponents()
{
	CharacterAttributeSet = AttributeSet;

	Super::PostInitializeComponents();

	InitializeGAS();

	GrantInitialSkills();

	ASC->SetNumericAttributeBase(AttributeSet->GetMaxHealthAttribute(), InitialAttributes.MaxHealth);
	ASC->SetNumericAttributeBase(AttributeSet->GetHealthAttribute(), InitialAttributes.CurrentHealth);
	ASC->SetNumericAttributeBase(AttributeSet->GetCombatSpeedAttribute(), InitialAttributes.CombatSpeed);
	ASC->SetNumericAttributeBase(AttributeSet->GetAttackPowerAttribute(), InitialAttributes.AttackPower);
	ASC->SetNumericAttributeBase(AttributeSet->GetDefenseAttribute(), InitialAttributes.Defense);
	ASC->SetNumericAttributeBase(AttributeSet->GetCriticalChanceAttribute(), InitialAttributes.CriticalChance);
	ASC->SetNumericAttributeBase(AttributeSet->GetCriticalDamageMultiplierAttribute(), InitialAttributes.CriticalDamageMultiplier);
	ASC->SetNumericAttributeBase(AttributeSet->GetDodgeChanceAttribute(), InitialAttributes.DodgeChance);

	// 이동용 캡슐은 자유조준 레이를 통과시킨다.
	GetCapsuleComponent()->SetCollisionResponseToChannel(CX_TRACE_COMBAT_AIM, ECR_Ignore);

	// Mesh는 자유조준 피격 판정에 사용한다.
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(CX_TRACE_COMBAT_AIM, ECR_Block);

	// 보스 HP는 전투 HUD에서 표시
	if (bIsBoss)
	{
		HpBar->SetVisibility(false);
	}
}

void ACXCombatCharacterEnemy::SelectActionPhase()
{
	if (!IsValid(ASC) || ExecutingCombatAbility.IsValid())
	{
		return;
	}

	SelectedAttackHandle = FGameplayAbilitySpecHandle();

	TArray<TPair<FGameplayAbilitySpecHandle, int32>> Candidates;
	int64 TotalWeight = 0;

	{
		FScopedAbilityListLock AbilityListLock(*ASC);

		if (InitialSkills.IsEmpty())
		{
			FGameplayTagContainer AttackTags;
			AttackTags.AddTag(CXVIGameplayTags::Ability_Basic_Attack);

			TArray<FGameplayAbilitySpec*> MatchingAbilities;
			ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
				AttackTags, MatchingAbilities, false);

			for (const FGameplayAbilitySpec* Spec : MatchingAbilities)
			{
				if (Spec && Spec->Ability && !Spec->IsActive())
				{
					Candidates.Emplace(Spec->Handle, 1);
					++TotalWeight;
				}
			}
		}
		else
		{
			const FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();

			for (const auto& Skill : SkillWeights)
			{
				const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Skill.Key);
				if (ActorInfo && Spec && Spec->Ability && !Spec->IsActive()
					&& Spec->Ability->CanActivateAbility(Spec->Handle, ActorInfo))
				{
					Candidates.Emplace(Skill.Key, Skill.Value);
					TotalWeight += Skill.Value;
				}
			}
		}
	}

	if (TotalWeight == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[적 스킬 선택] %s: 사용 가능한 스킬 없음"), *GetName());

		SavedMessage.Character = this;
		SavedMessage.MessageType = ECXCombatMessageType::SkippedTurn;
		CombatMessageResponse(SavedMessage);
		return;
	}

	int64 Roll = FMath::RandRange(int64{ 1 }, TotalWeight);
	for (const auto& Candidate : Candidates)
	{
		Roll -= Candidate.Value;
		if (Roll <= 0)
		{
			SelectedAttackHandle = Candidate.Key;
			break;
		}
	}

	Super::SelectActionPhase();
}

void ACXCombatCharacterEnemy::SelectTargetPhase()
{
	SelectedCombatTargets.Reset();
	CurrentTargetPawn.Reset();

	// 매니저가 전달한 후보에서 유효하고 살아 있는 대상만 수집.
	TArray<TWeakObjectPtr<ACXCombatCharacterBase>> AliveTargets;

	for (const TWeakObjectPtr<APawn>& Candidate : SavedMessage.Targets)
	{
		ACXCombatCharacterBase* Target =
			Cast<ACXCombatCharacterBase>(Candidate.Get());

		if (!IsValid(Target) || !Target->GetIsCharacterAlive())
		{
			continue;
		}

		AliveTargets.AddUnique(Target);
	}

	const FGameplayAbilitySpec* Spec = GetSelectedAttackSpec();

	const UCXGameplayAbilityCombatBase* AttackAbility = Spec
		? Cast<UCXGameplayAbilityCombatBase>(Spec->Ability)
		: nullptr;

	const ECXAttackTargetMode TargetMode = AttackAbility
		? AttackAbility->GetAttackTargetMode()
		: ECXAttackTargetMode::Single;

	if (!AliveTargets.IsEmpty())
	{
		if (TargetMode == ECXAttackTargetMode::AllOpponents)
		{
			SelectedCombatTargets = AliveTargets;
			CurrentTargetPawn = SelectedCombatTargets[0];
		}
		else
		{
			const int32 TargetIndex =
				FMath::RandRange(0, AliveTargets.Num() - 1);

			// BT와 카메라가 사용할 대표 대상.
			CurrentTargetPawn = AliveTargets[TargetIndex].Get();
			SelectedCombatTargets.Add(AliveTargets[TargetIndex].Get());
			
		}


		if (const UCXCombatManagerSubsystem* CombatManager =
			UCXCombatManagerSubsystem::TryGet(this))
		{
			if (IsBoss())
			{
				CombatManager->RequestCameraPreset(TEXT("BossMonster.Attack"));
			}
			else
			{
				CombatManager->RequestCameraPreset(TEXT("Monster.Attack"));
			}
		}
	}

	UE_LOG(LogTemp, Display,
		TEXT("[대상 선택] 공격자=%s, 대상 수=%d"),
		*GetName(),
		SelectedCombatTargets.Num());

	for (const auto& Target : SelectedCombatTargets)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[대상 선택] 대상=%s"),
			*GetNameSafe(Target.Get()));
	}

	// 대상이 없더라도 다음 단계로 전달.
	// 기존 ActionPhase의 대상 검사에서 실패 처리.
	FCXCombatMessageData Response = SavedMessage;
	Response.Character = this;
	Response.MessageType = ECXCombatMessageType::None;

	CombatMessageResponse(Response);
}

const FGameplayAbilitySpec* ACXCombatCharacterEnemy::GetSelectedAttackSpec() const
{

	if (!IsValid(ASC) || !SelectedAttackHandle.IsValid())
	{
		return nullptr;
	}

	return ASC->FindAbilitySpecFromHandle(SelectedAttackHandle);
}

ECXActionType ACXCombatCharacterEnemy::GetSelectedActionType() const
{
	return SkillDisplayNames.Contains(SelectedAttackHandle) ? ECXActionType::Skill : ECXActionType::BasicAttack;
}

FText ACXCombatCharacterEnemy::GetSelectedActionName() const
{
	const FText* Name = SkillDisplayNames.Find(SelectedAttackHandle);

	return Name ? *Name : FText::GetEmpty();
}

bool ACXCombatCharacterEnemy::ExecuteCombatAbility()
{
	return TryStartCombatAbility(SelectedAttackHandle);
}

void ACXCombatCharacterEnemy::GrantInitialSkills()
{
	if (!HasAuthority() || !IsValid(ASC))
	{
		return;
	}

	for (const FCXEnemySkillSpawnEntry& Skill : InitialSkills)
	{
		if (Skill.Weight <= 0)
		{
			continue;
		}

		const TSubclassOf<UGameplayAbility> SkillClass = Skill.AbilityClass.Get();
		if (!SkillClass || SkillClass->HasAnyClassFlags(CLASS_Abstract))
		{
			UE_LOG(LogTemp, Warning, TEXT("[적 스킬 등록 실패] %s / %s"),
				*GetName(), *Skill.AbilityClass.ToString());
			continue;
		}

		const FGameplayAbilitySpec* ExistingSpec = ASC->FindAbilitySpecFromClass(SkillClass);

		const FGameplayAbilitySpecHandle Handle = ExistingSpec ? ExistingSpec->Handle : ASC->GiveAbility(FGameplayAbilitySpec(SkillClass));

		if (Handle.IsValid())
		{
			SkillWeights.Add(Handle, Skill.Weight);
			SkillDisplayNames.Add(Handle, Skill.DisplayName);
		}
	}
}

void ACXCombatCharacterEnemy::StartDodgeMontage()
{
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (nullptr == AnimInstance)
	{
		return;
	}
	AnimInstance->Montage_Play(DodgeMontage);
}

void ACXCombatCharacterEnemy::StartHitReactMontage()
{
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (nullptr == AnimInstance)
	{
		return;
	}
	AnimInstance->Montage_Play(HitReactMontage);
}

void ACXCombatCharacterEnemy::PlayDeathPresentation()
{
	USkeletalMeshComponent* BossMesh = GetMesh();
	if (!IsBoss() || !BossMesh || !BossMesh->GetPhysicsAsset())
	{
		return;
	}

	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BossMesh->SetCollisionProfileName(TEXT("Ragdoll"));
	BossMesh->SetSimulatePhysics(true);
}

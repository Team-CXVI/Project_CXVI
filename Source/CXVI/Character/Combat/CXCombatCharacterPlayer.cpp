// Fill out your copyright notice in the Description page of Project Settings.


#include "CXCombatCharacterPlayer.h"

#include "CXVI/Tags/CXGameplayTags.h"
#include "Foundation/Combat/CXCollisionChannels.h"

#include "CXVI/AbilitySystem/Attributes/Combat/CXCombatPlayerAttributeSet.h"
#include "AbilitySystem/Abilities/Combat/CXGA_UseItemBase.h"
#include "Character/Combat/Components/CXJustTimingComponent.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatCharacterPlayer)

ACXCombatCharacterPlayer::ACXCombatCharacterPlayer()
{
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->SetRelativeLocation(FVector(0.f, 30.f, 50.f));
	SpringArm->TargetArmLength = 400.f;

	SpringArm->bUsePawnControlRotation = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CharacterMeshRef(
		TEXT("/Game/Combat/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple")
	);

	if (CharacterMeshRef.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(CharacterMeshRef.Object);
	}

	JustTimingComponent = CreateDefaultSubobject<UCXJustTimingComponent>(TEXT("JustTimingComponent"));

	AttributeSet = CreateDefaultSubobject<UCXCombatPlayerAttributeSet>(TEXT("AttributeSet"));
	ASC->AddAttributeSetSubobject<UCXCombatPlayerAttributeSet>(AttributeSet);
	

	//static ConstructorHelpers::FClassFinder<UAnimInstance> CharacterAnimRef(
	//	TEXT("/Game/CXVI/Characters/Combat/Animation/ABP_Combat_Character.ABP_Combat_Character_C")
	//);
	//if (CharacterAnimRef.Succeeded())
	//{
	//	GetMesh()->SetAnimInstanceClass(CharacterAnimRef.Class);
	//}
}


void ACXCombatCharacterPlayer::Look(const FInputActionValue& Value)
{
	// 입력 값 읽어오기 ( 입력에 지정된 타입으로 변환 ).
	FVector2D RotationValue = Value.Get<FVector2D>();

	// 회전 처리 -> 컨트롤러에 회전 입력 전달.
	FRotator CharacterRotation = GetActorRotation();
	CharacterRotation.Yaw += RotationValue.X;
	//CharacterRotation.Pitch += RotationValue.Y;
	SetActorRotation(CharacterRotation);

	FRotator CameraRotation = SpringArm->GetRelativeRotation();
	CameraRotation.Pitch += RotationValue.Y;
	SpringArm->SetRelativeRotation(CameraRotation);
}

bool ACXCombatCharacterPlayer::PerformAimTrace(FHitResult& OutHit)
{
	OutHit = FHitResult();

	UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}


	const float TraceDistance = 10000.f;
	const FVector TraceStart = Camera->GetComponentLocation();
	const FVector TraceEnd = TraceStart + Camera->GetForwardVector() * TraceDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = World->LineTraceSingleByChannel(
		OutHit,
		TraceStart,
		TraceEnd,
		CX_TRACE_COMBAT_AIM,
		QueryParams
	);

	const FVector DebugEnd = bHit ? OutHit.ImpactPoint : TraceEnd;

	DrawDebugLine(
		World,
		TraceStart,
		TraceEnd,
		bHit ? FColor::Green : FColor::Red,
		false,
		2.f,
		0,
		1.f
	);

	if (bHit)
	{
		UE_LOG(LogTemp, Display, 
			TEXT("[자유조준] 명중: Actor=%s, Component=%s, Bone=%s"), 
			*GetNameSafe(OutHit.GetActor()), 
			*GetNameSafe(OutHit.GetComponent()), 
			*OutHit.BoneName.ToString());


		DrawDebugSphere(
			World,
			OutHit.ImpactPoint,
			8.f,
			12,
			FColor::Yellow,
			false,
			2.f
		);
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("[자유조준] 명중 대상 없음"));
	}


	return bHit;
}

bool ACXCombatCharacterPlayer::ToggleAimMode()
{
	bIsAimMode = !bIsAimMode;

	ReturnToOriginLocation();

	return bIsAimMode;
}

void ACXCombatCharacterPlayer::FireAbility()
{
	if (!ASC)
	{
		return;
	}

	// Ability 처리중 부여 목록이 변경되어도 안전하도록 보호합니다.
	FScopedAbilityListLock AbilityListLock(*ASC);

	FGameplayTagContainer AttackTags;
	AttackTags.AddTag(CXVIGameplayTags::Ability_Fire);

	TArray<FGameplayAbilitySpec*> MatchingAbilities;

	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
		AttackTags,
		MatchingAbilities,
		false
	);

	for (FGameplayAbilitySpec* Spec : MatchingAbilities)
	{
		if (!Spec || !Spec->Ability || Spec->IsActive())
		{
			continue;
		}


		// 스킬 사용중인건 배열에 저장
		CurrentActivatingHandles.AddUnique(Spec->Handle);

		// 첫 클릭
		if (!ASC->TryActivateAbility(Spec->Handle))
		{
			CurrentActivatingHandles.Remove(Spec->Handle);

			UE_LOG(LogTemp, Display, TEXT("[자유조준] 발사 불가: AP 또는 사용 조건 확인"));
		}
		// 입력 한번에 발사 어빌리티 하나만 시도
		break;
	}
}

bool ACXCombatCharacterPlayer::CanActivateSelectSkill(int32 SlotIndex) const
{
	if (!IsValid(ASC) || !ASC->AbilityActorInfo.IsValid() ||
		!SkillAbilities.IsValidIndex(SlotIndex) || !SkillAbilities[SlotIndex])
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 스킬 슬롯 설정을 확인해주세요. 슬롯=%d"), *GetName(), SlotIndex);
		return false;
	}

	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(SkillAbilities[SlotIndex]);

	if (!Spec || !Spec->Ability)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 선택한 스킬이 ASC에 부여되어있지 않음."), *GetName());
		return false;
	}

	if (Spec->IsActive())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 이미 실행중인 스킬입니다."), *GetName());
		return false;
	}

	const UGameplayAbility* Ability = Spec->Ability.Get();

	const FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();

	// 실제 차감에 사용할 비용 GE를 기준으로 검사
	// 이 호출은 마나를 소비하지 않음
	if (!Ability->CheckCost(Spec->Handle, ActorInfo))
	{
		UCXCombatPlayerAttributeSet* PlayerAttribute = Cast<UCXCombatPlayerAttributeSet>(AttributeSet);

		const float CurrentMana = PlayerAttribute ? PlayerAttribute->GetAP() : 0.f;

		UE_LOG(LogTemp, Display, TEXT("%s: 마나 부족으로 스킬을 사용할 수  없습니다. 현재 마나 =%.0f"), *GetName(), CurrentMana);
		
		return false;
	}

	// 비용 외에도 쿨다운 태그 등 GAS의 활성화 조건을 확인한다.
	if (!Ability->CanActivateAbility(Spec->Handle, ActorInfo))
	{
		UE_LOG(LogTemp, Display, TEXT("%s: 현재 상태에서는 스킬을 사용할수 없습니다."), *GetName());
		return false;
	}

	return true;
}

void ACXCombatCharacterPlayer::SkipTurn()
{
	EndTurnPhase();
}

void ACXCombatCharacterPlayer::TryRunaway()
{
	
}

bool ACXCombatCharacterPlayer::CanUseHealingItem(FName ItemId, const ACXCombatCharacterBase* Target) const
{
	if (!IsValid(ASC))
	{
		return false;
	}

	FScopedAbilityListLock AbilityListLock(*ASC);

	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(CXVIGameplayTags::Ability_Item_Heal);

	TArray<FGameplayAbilitySpec*> MatchingAbilities;

	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
		AbilityTags,
		MatchingAbilities,
		false
	);

	if (MatchingAbilities.Num() != 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("[아이템] 회복 GA 등록 확인 필요: 검색 결과 %d개"), MatchingAbilities.Num());

		return false;
	}

	const FGameplayAbilitySpec* Spec = MatchingAbilities[0];
	
	if (!Spec || Spec->IsActive())
	{
		return false;
	}

	const UCXGA_UseItemBase* ItemAbility = Cast<UCXGA_UseItemBase>(Spec->Ability);

	return ItemAbility && ItemAbility->CanUseItem(this, ItemId, Target);
}

void ACXCombatCharacterPlayer::PostInitializeComponents()
{
	CharacterAttributeSet = AttributeSet;

	Super::PostInitializeComponents();

	InitializeGAS();

	ASC->SetNumericAttributeBase(AttributeSet->GetMaxHealthAttribute(), 200.f);
	ASC->SetNumericAttributeBase(AttributeSet->GetHealthAttribute(), 100.f);
	ASC->SetNumericAttributeBase(AttributeSet->GetCombatSpeedAttribute(), 200.f);
	ASC->SetNumericAttributeBase(AttributeSet->GetAttackPowerAttribute(), 40.f);
	ASC->SetNumericAttributeBase(AttributeSet->GetDefenseAttribute(), 40.f);
	ASC->SetNumericAttributeBase(AttributeSet->GetCriticalChanceAttribute(), 50.f);
	ASC->SetNumericAttributeBase(AttributeSet->GetCriticalDamageMultiplierAttribute(), 2.f);
}

bool ACXCombatCharacterPlayer::ActivateSelectedSkill(int32 SlotIndex)
{
	if (!ASC ||
		!SkillAbilities.IsValidIndex(SlotIndex) ||
		!SkillAbilities[SlotIndex])
	{
		return false;
	}

	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(SkillAbilities[SlotIndex]);

	if (!Spec || Spec->IsActive())
	{
		return false;
	}

	return Spec ? TryStartCombatAbility(Spec->Handle) : false;
}

bool ACXCombatCharacterPlayer::ExecuteCombatAbility()
{
	const FCXActionSelection Action = GetPendingAction();

	switch (Action.Type)
	{
	case ECXActionType::BasicAttack:
		return BasicAttackAbility();

	case ECXActionType::Skill:
		return ActivateSelectedSkill(Action.SlotIndex);

	case ECXActionType::CounterAttack:
		return CounterAbility();

	case ECXActionType::Item:
		// 나중에 종류 별로 설정
		return ActivateHealingItem();

	default:
		return false;
	}
}

void ACXCombatCharacterPlayer::SelectActionPhase()
{
	// 커맨드 폰의 SubmitAction() 기다리는 중
}

void ACXCombatCharacterPlayer::SelectTargetPhase()
{

	OnTargetSelectionRequested.Broadcast(this, GetTargetCandidates());

	// 대상 확정 전이므로 응답하지 않음.
}

bool ACXCombatCharacterPlayer::ActivateHealingItem()
{
	if (!IsValid(ASC))
	{
		return false;
	}

	FGameplayAbilitySpecHandle AbilityHandle;
	
	{
		FScopedAbilityListLock AbilityListLock(*ASC);

		FGameplayTagContainer AbilityTags;
		AbilityTags.AddTag(CXVIGameplayTags::Ability_Item_Heal);

		TArray<FGameplayAbilitySpec*> MatchingAbilities;

		ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
			AbilityTags,
			MatchingAbilities,
			false
		);

		// 회복 아이템 실행 태그는 하나의 GA에만 지정
		if (MatchingAbilities.Num() != 1)
		{
			UE_LOG(LogTemp, Warning, TEXT("[아이템] 회복 Ga 등록 확인 필요: 검색결과 %d개"), MatchingAbilities.Num());

			return false;
		}

		const FGameplayAbilitySpec* Spec = MatchingAbilities[0];

		if (!Spec || !Spec->Ability || Spec->IsActive())
		{
			return false;
		}

		AbilityHandle = Spec->Handle;
	}

	UE_LOG(LogTemp, Display, TEXT("[아이템] 회복 GA 실행 요청"));

	return TryStartCombatAbility(AbilityHandle);
}

bool ACXCombatCharacterPlayer::ShouldApproachTarget() const
{
	if (GetPendingAction().Type == ECXActionType::Item)
	{
		return false;
	}

	return Super::ShouldApproachTarget();
}

//void ACXCombatCharacterPlayer::ActionPhase()
//{
//	const FCXActionSelection Action = GetPendingAction();
//
//	switch (Action.Type)
//	{
//	case ECXActionType::BasicAttack:
//		if (!BasicAttackAbility())
//		{
//			FinishAction();
//		}
//		return;
//
//	case ECXActionType::Skill:
//		if (!ActivateSelectedSkill(Action.SlotIndex))
//		{
//			FinishAction();
//		}
//		return;
//
//	default:
//		return;
//	}
//}

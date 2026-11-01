// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Character/Combat/CXCombatCharacterEnemy.h"

#include "CXVI/AbilitySystem/Attributes/Combat/CXCombatEnemyAttributeSet.h"
#include "Foundation/Combat/CombatMessage/CXCombatMessageData.h"

#include "Foundation/Combat/CXCollisionChannels.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatCharacterEnemy)


ACXCombatCharacterEnemy::ACXCombatCharacterEnemy()
{
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -88.f), FRotator(0.f, -90.f, 0.f));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CharacterMeshRef(
		TEXT("/Game/Combat/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")
	);

	if (CharacterMeshRef.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(CharacterMeshRef.Object);
	}

	//GetCapsuleComponent()->SetCollisionResponseToChannel(CX_TRACE_COMBAT_AIM, ECR_Block);

	AttributeSet = CreateDefaultSubobject<UCXCombatEnemyAttributeSet>(TEXT("AttributeSet"));
	ASC->AddAttributeSetSubobject<UCXCombatEnemyAttributeSet>(AttributeSet);

	
	//CharacterAttributeSet = AttributeSet;

	/*static ConstructorHelpers::FClassFinder<UAnimInstance> CharacterAnimRef(
		TEXT("/Game/CXVI/Characters/Combat/Animation/ABP_Combat_Character.ABP_Combat_Character_C")
	);

	if (CharacterAnimRef.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(CharacterAnimRef.Class);
	}*/
}

void ACXCombatCharacterEnemy::PostInitializeComponents()
{
	CharacterAttributeSet = AttributeSet;

	Super::PostInitializeComponents();

	InitializeGAS();

	ASC->SetNumericAttributeBase(AttributeSet->GetMaxHealthAttribute(), 300.f);
	ASC->SetNumericAttributeBase(AttributeSet->GetHealthAttribute(), 300.f);
	ASC->SetNumericAttributeBase(AttributeSet->GetCombatSpeedAttribute(), 100.f);
	ASC->SetNumericAttributeBase(AttributeSet->GetAttackPowerAttribute(), 40.f);
	ASC->SetNumericAttributeBase(AttributeSet->GetDefenseAttribute(), 20.f);
	ASC->SetNumericAttributeBase(AttributeSet->GetCriticalChanceAttribute(), 20.f);
	ASC->SetNumericAttributeBase(AttributeSet->GetCriticalDamageMultiplierAttribute(), 2.f);
	ASC->SetNumericAttributeBase(AttributeSet->GetDodgeChanceAttribute(), 30.f);

	// 이동용 캡슐은 자유조준 레이를 통과시킨다.
	GetCapsuleComponent()->SetCollisionResponseToChannel(CX_TRACE_COMBAT_AIM, ECR_Ignore);

	// Mesh는 자유조준 피격 판정에 사용한다.
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(CX_TRACE_COMBAT_AIM, ECR_Block);
}

void ACXCombatCharacterEnemy::SelectTargetPhase()
{
	// 이전 페이즈에서 누군가를 선택해야하는 선택지를 골랐을경우

	// 적이나 아군중에 선택을 해야함

	const int32 MaxTargetNum = SavedMessage.Targets.Num();

	const int32 RandTargetIndex = FMath::RandRange(0, MaxTargetNum - 1);

	for (int32 Index = 0; Index < MaxTargetNum; ++Index)
	{
		if (ACXCombatCharacterBase* Target = Cast<ACXCombatCharacterBase>(SavedMessage.Targets[Index].Get()))
		{
			if (!Target->GetIsCharacterAlive())
			{
				continue;
			}

			if (Index == RandTargetIndex)
			{
				CurrentTargetPawn = Target;
			}
		}
	}


	// 선택을 하지않고 취소를 할경우 이전 페이즈로 돌아간다.

	SavedMessage.Character = this;
	SavedMessage.MessageType = ECXCombatMessageType::None;
	CombatMessageResponse(SavedMessage);
}

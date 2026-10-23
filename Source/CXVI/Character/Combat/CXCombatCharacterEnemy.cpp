// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Character/Combat/CXCombatCharacterEnemy.h"

#include "CXVI/AbilitySystem/Attributes/Combat/CXCombatAttributeSet.h"

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
	Super::PostInitializeComponents();

	InitializeGAS();

	ASC->SetNumericAttributeBase(CharacterAttributeSet->GetMaxHealthAttribute(), 300.f);
	ASC->SetNumericAttributeBase(CharacterAttributeSet->GetHealthAttribute(), 300.f);
	ASC->SetNumericAttributeBase(CharacterAttributeSet->GetCombatSpeedAttribute(), 100.f);
}

void ACXCombatCharacterEnemy::ActionPhase()
{
	Super::ActionPhase();

	BasicAttackAbility();
}

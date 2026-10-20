// Fill out your copyright notice in the Description page of Project Settings.


#include "CXCombatCharacterBase.h"

#include "..\..\Tags\CXGameplayTags.h"

#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"



// Sets default values
ACXCombatCharacterBase::ACXCombatCharacterBase()
{
	// Pawn
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	//GetCapsuleComponent()->SetCollisionProfileName(TEXT(""));

	// Movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 300.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;

	// Mesh
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -100.0f), FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	GetMesh()->SetCollisionProfileName(TEXT("NoCollision"));

	// Widget
	HpBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("HpBar"));
	HpBar->SetupAttachment(GetMesh());
	HpBar->SetRelativeLocation(FVector(0.f, 0.f, 180.f));
	HpBar->SetWidgetSpace(EWidgetSpace::Screen);
	HpBar->SetDrawSize(FVector2D(150.f, 20.f));
	HpBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// ASC
	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));

	// AttributeSet
	/*CharacterAttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("CharacterAttributeSet"));
	ASC->AddAttributeSetSubobject<UBaseAttributeSet>(CharacterAttributeSet);

	CombatAttributeSet = CreateDefaultSubobject<UGASCombatAttributeSet>(TEXT("CombatAttributeSet"));
	ASC->AddAttributeSetSubobject<UGASCombatAttributeSet>(CombatAttributeSet);*/

	// 캐릭터 기본변수 초기화
	bIsCharacterAlive = true;
	bIsAimMode = false;

}

// Called when the game starts or when spawned
void ACXCombatCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	InitializeGAS();
	
}

void ACXCombatCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	CXPostInitializeComponents();

	/*ASC->GetSet<UBaseAttributeSet>()->OnOutOfHealth.AddDynamic(this, &CXCombatCharacterBase::OnOutOfHealthCpp);

	ASC->SetNumericAttributeBase(CharacterAttributeSet->GetMaxHealthAttribute(), 200.f);
	ASC->SetNumericAttributeBase(CharacterAttributeSet->GetHealthAttribute(), 200.f);
	ASC->SetNumericAttributeBase(CharacterAttributeSet->GetSpeedAttribute(), 200.f);*/
}

void ACXCombatCharacterBase::InitializeGAS()
{
	if (!IsValid(ASC))
	{
		return;
	}

	// 빙의 후 컨트롤러 정보가 달라질 수 있으므로 다시 갱신합니다.
	ASC->InitAbilityActorInfo(this, this);

	// 어빌리티 부여는 서버에서 한 번만 처리합니다.
	if (!HasAuthority() || bIsInitializedAbility)
	{
		return;
	}

	bIsInitializedAbility = true;

	for (const auto& Ability : Abilities)
	{
		if (Ability)
		{
			ASC->GiveAbility(FGameplayAbilitySpec(Ability));
		}
	}
}

void ACXCombatCharacterBase::NotifyBattleAbilityEnded(FGameplayAbilitySpecHandle Handle, bool bWasCancelled)
{
}

UAbilitySystemComponent* ACXCombatCharacterBase::GetAbilitySystemComponent() const
{
	return nullptr;
}

void ACXCombatCharacterBase::OnOutOfHealthCpp(AActor* InInstigator)
{
	HpBar->SetVisibility(false);

	bIsCharacterAlive = false;
}

float ACXCombatCharacterBase::GetCombatSpeed() const
{
	return 0.0f;
}

bool ACXCombatCharacterBase::ToggleAimMode()
{
	bIsAimMode = !bIsAimMode;

	ReturnToOriginLocation();

	return bIsAimMode;
}

void ACXCombatCharacterBase::BasicAttackAbility()
{
	if (!ASC)
	{
		return;
	}

	// Ability 처리중 부여 목록이 변경되어도 안전하도록 보호합니다.
	FScopedAbilityListLock AbilityListLock(*ASC);

	FGameplayTagContainer AttackTags;
	AttackTags.AddTag(CXVIGameplayTags::Ability_Basic_Attack);

	TArray<FGameplayAbilitySpec*> MatchingAbilities;

	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
		AttackTags,
		MatchingAbilities,
		false
	);

	for (FGameplayAbilitySpec* Spec : MatchingAbilities)
	{
		if (!Spec || !Spec->Ability)
		{
			continue;
		}

		Spec->InputPressed = true;

		if (Spec->IsActive())
		{
			// 공격중 추가 클릭
			ASC->AbilitySpecInputPressed(*Spec);
		}
		else
		{
			// 먼저 적앞으로 이동
			TeleportToAttackLocation();


			// 스킬 사용중인건 배열에 저장
			CurrentActivatingHandles.Add(Spec->Handle);

			// 첫 클릭
			ASC->TryActivateAbility(Spec->Handle);
		}
	}
}

void ACXCombatCharacterBase::TeleportToAttackLocation()
{
	APawn* Target = CurrentTargetPawn.Get();

	if (!IsValid(Target))
	{
		return;
	}

	FVector AttackPosition = Target->GetActorLocation() + Target->GetActorForwardVector() * 50.f;

	SetActorLocation(AttackPosition);
}

void ACXCombatCharacterBase::ReturnToOriginLocation()
{
	SetActorTransform(OriginTransform);
}


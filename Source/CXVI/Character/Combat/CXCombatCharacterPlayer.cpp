// Fill out your copyright notice in the Description page of Project Settings.


#include "CXCombatCharacterPlayer.h"

#include "CXVI/Tags/CXGameplayTags.h"

#include "CXVI/AbilitySystem/Attributes/CXCombatAttributeSet.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "AbilitySystemComponent.h"

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

	FVector ViewLocation = Camera->GetComponentLocation();

	const float TraceDistance = 10000.f;

	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + Camera->GetForwardVector() * TraceDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = World->LineTraceSingleByChannel(
		OutHit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
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
		if (!Spec || !Spec->Ability)
		{
			continue;
		}


		// 스킬 사용중인건 배열에 저장
		CurrentActivatingHandles.Add(Spec->Handle);

		// 첫 클릭
		ASC->TryActivateAbility(Spec->Handle);

	}
}

void ACXCombatCharacterPlayer::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	InitializeGAS();

	ASC->SetNumericAttributeBase(CharacterAttributeSet->GetMaxHealthAttribute(), 200.f);
	ASC->SetNumericAttributeBase(CharacterAttributeSet->GetHealthAttribute(), 200.f);
	ASC->SetNumericAttributeBase(CharacterAttributeSet->GetCombatSpeedAttribute(), 200.f);
}

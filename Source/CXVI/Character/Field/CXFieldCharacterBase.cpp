// Fill out your copyright notice in the Description page of Project Settings.


#include "CXFieldCharacterBase.h"
#include "CXVI/Tags/CXFieldTags.h"

#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldCharacterBase)

// Sets default values
ACXFieldCharacterBase::ACXFieldCharacterBase()
{

	// ASC 생성.
	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));

	// 라이팅 채널 : 0(기본 라이트) + 1(필드 캐릭터 전용 라이트, 필드 레벨의 DL_Field_CharacterKey). 필드 적도 같은 채널
	GetMesh()->SetLightingChannels(true, true, false);

}

// Called when the game starts or when spawned
void ACXFieldCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// GameplayAbility 등록.
	if (ASC)
	{
		ASC->InitAbilityActorInfo(this, this);

	}
}

void ACXFieldCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (!IsValid(ASC))
	{
		ASC = FindComponentByClass<UAbilitySystemComponent>();
	}
}




void ACXFieldCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// ASC의 OwnerActor 설정
	if (ASC)
	{
		ASC->InitAbilityActorInfo(this, this);
	}
}

UAbilitySystemComponent* ACXFieldCharacterBase::GetAbilitySystemComponent() const
{
	return ASC;
}




// Fill out your copyright notice in the Description page of Project Settings.


#include "CXFieldCharacterBase.h"
#include "CXVI/Tags/CXFieldTags.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldCharacterBase)

// Sets default values
ACXFieldCharacterBase::ACXFieldCharacterBase()
{

	// ASC 생성.
	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));


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




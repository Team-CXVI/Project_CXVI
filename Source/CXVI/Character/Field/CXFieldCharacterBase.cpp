// Fill out your copyright notice in the Description page of Project Settings.


#include "CXFieldCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "../../AbilitySystem/Attributes/CXBaseAttributeSet.h"
#include "../../Tags/CXFieldTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldCharacterBase)

// Sets default values
ACXFieldCharacterBase::ACXFieldCharacterBase(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

	// ASC 생성.
	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));

	// Attributeset 컴포넌트처럼 등록하기.
	BaseAttributeSet = CreateDefaultSubobject<UCXBaseAttributeSet>(TEXT("BaseAttributeSet"));

}

// Called when the game starts or when spawned
void ACXFieldCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// GameplayAbility 등록.
	if (ASC)
	{
		// 컨트롤러가 없는 에너미도 GAS 소유자 정보를 갖도록 공통 초기화.
		ASC->InitAbilityActorInfo(this, this);

		// 공격 Notify의 GameplayEvent를 받아 실제 충돌 검사를 실행.
		//if (!HitTraceDelegateHandle.IsValid())
		//{
		//	HitTraceDelegateHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(CXHitTraceTag)
		//		.AddUObject(this, &ThisClass::HitTrace);
		//}

		if (BaseAttributeInitEffect)
		{
			// GameplayAbility와 동일하게 원본 형태로 사용하는 것이 아닌 FGameplayEffectSpecHandle구조체로 변환후 ASC 등록 사용.
			FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
			EffectContext.AddSourceObject(this);

			FGameplayEffectSpecHandle BaseEffectSpecHandle = ASC->MakeOutgoingSpec(BaseAttributeInitEffect, 1, EffectContext);
			if (BaseEffectSpecHandle.IsValid())
			{
				// Handle을 가지고 타겟의 ASC에 Effect 적용. (GA의 Activate 처리).
				// BaseEffectSpecHandle.Data -> TSharedPtr<UGameplayEffectSpec>
				ASC->ApplyGameplayEffectSpecToTarget(*BaseEffectSpecHandle.Data.Get(), ASC);
			}
		}
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




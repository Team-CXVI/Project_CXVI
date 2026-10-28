// Fill out your copyright notice in the Description page of Project Settings.
#include "CXGrapplingHookTriggerBox.h"
#include "CXVI/Character/Field/CXFieldPlayerCharacter.h"
#include "CXVI/Tags/CXFieldTags.h"

#include "AbilitySystemComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/WidgetComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGrapplingHookTriggerBox) 

ACXGrapplingHookTriggerBox::ACXGrapplingHookTriggerBox(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	OnActorBeginOverlap.AddDynamic(this, &ThisClass::HandleActorBeginOverlap);
	OnActorEndOverlap.AddDynamic(this, &ThisClass::HandleActorEndOverlap);

	// 트리거 충돌 설정.
	if (UShapeComponent* Collision = GetCollisionComponent())
	{
		Collision->SetCollisionProfileName(TEXT("Trigger"));
		Collision->SetGenerateOverlapEvents(true);
	}

	GrapplingUI =
		CreateDefaultSubobject<UWidgetComponent>(TEXT("GrapplingPrompt"));

	GrapplingUI->SetupAttachment(GetRootComponent());
	GrapplingUI->SetWidgetSpace(EWidgetSpace::Screen);
	GrapplingUI->SetDrawSize(FVector2D(100.0f, 40.0f));
	GrapplingUI->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 연결이 끝나기 전까지 새 UI는 숨겨 둡니다.
	GrapplingUI->SetVisibility(false);

}

void ACXGrapplingHookTriggerBox::BeginPlay()
{
	Super::BeginPlay();

	if (!IsValid(GrapplingUI) || !IsValid(GrapplingTarget))
	{
		return;
	}

	USceneComponent* TargetRoot = GrapplingTarget->GetRootComponent();
	if (!IsValid(TargetRoot))
	{
		return;
	}

	GrapplingUI->AttachToComponent(TargetRoot, FAttachmentTransformRules::SnapToTargetIncludingScale);

	GrapplingUI->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));

	// 트리거나 타깃의 크기 때문에 UI가 확대되지 않는다.
	GrapplingUI->SetWorldScale3D(FVector::OneVector);

	GrapplingUI->InitWidget();
}

void ACXGrapplingHookTriggerBox::HandleActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	ACXFieldPlayerCharacter* PlayerCharacter = Cast<ACXFieldPlayerCharacter>(OtherActor);

	// 유효성 검사.
	if (!PlayerCharacter || !IsValid(GrapplingTarget) || !IsValid(LandingSurface))
	{
		return;
	}


	if (UAbilitySystemComponent* ASC = PlayerCharacter->GetAbilitySystemComponent())
	{
		
		ASC->RemoveLooseGameplayTag(CXVIFieldTag::State_CanGrapplingHook);
	
		if (PlayerCharacter->GetGrapplingTargetActor() == GrapplingTarget.Get())
		{
			PlayerCharacter->SetGrapplingTarget(nullptr);
		}
	}
}

void ACXGrapplingHookTriggerBox::HandleActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	ACXFieldPlayerCharacter* PlayerCharacter = Cast<ACXFieldPlayerCharacter>(OtherActor);

	// 유효성 검사.
	if (!PlayerCharacter || !IsValid(GrapplingTarget) || !IsValid(LandingSurface))
	{
		return;
	}

	
	if (UAbilitySystemComponent* ASC = PlayerCharacter->GetAbilitySystemComponent())
	{
		// 태그 변경 보다 먼저 대상 지정.
		PlayerCharacter->SetGrapplingTarget(GrapplingTarget.Get(), LandingSurface.Get());

		ASC->AddLooseGameplayTag(CXVIFieldTag::State_CanGrapplingHook);
	}
}

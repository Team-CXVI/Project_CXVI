// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/Field/Interactions/CXInteractableGimmick.h"

#include <Components/BoxComponent.h>
#include <Components/WidgetComponent.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXInteractableGimmick)

DEFINE_LOG_CATEGORY(LogCXGimmic);

// Sets default values
ACXInteractableGimmick::ACXInteractableGimmick()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	InteractionTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionTrigger"));
	InteractionTrigger->SetupAttachment(Root);
	InteractionTrigger->SetBoxExtent(FVector(100.f));
	InteractionTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionTrigger->SetCollisionObjectType(INTERACTABLE_CHANNEL);
	InteractionTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	// 송신기 구의 Object Type(WorldDynamic)에 Overlap. 양쪽이 서로 Overlap이어야 이벤트가 발생
	InteractionTrigger->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	InteractionTrigger->SetGenerateOverlapEvents(true);

	PromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PromptWidget"));
	PromptWidget->SetupAttachment(Root);
	PromptWidget->SetWidgetSpace(EWidgetSpace::Screen);
	PromptWidget->SetDrawAtDesiredSize(true);
	PromptWidget->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	PromptWidget->SetVisibility(false);
}

bool ACXInteractableGimmick::CanInteract_Implementation(AActor* Interactor) const
{
	if (bRequireInteractorFacing)
	{
		// 높이 차이는 무시하고 수평(XY) 방향만 비교
		const FVector ToGimmick = (GetActorLocation() - Interactor->GetActorLocation()).GetSafeNormal2D();
		if (ToGimmick.IsNearlyZero())
		{
			return bInteractionEnabled; // 수평으로 같은 위치면 방향 판단 불가 -> 통과
		}

		const FVector InteractorForward = Interactor->GetActorForwardVector().GetSafeNormal2D();
		const float MinDot = FMath::Cos(FMath::DegreesToRadians(InteractorFacingHalfAngle));
		if (FVector::DotProduct(InteractorForward, ToGimmick) < MinDot)
		{
			return false;
		}
	}

	return bInteractionEnabled;
}

void ACXInteractableGimmick::Interact_Implementation(AActor* Interactor)
{
	// Interactor가 상호작용한 캐릭터. 캐릭터에 구현된 동작 함수는 자식 기믹에서
	// Cast<캐릭터클래스>(Interactor)->함수() 로 호출하거나, BP의 OnInteracted에서 캐스팅해서 호출한다.
	K2_OnInteracted(Interactor);
}

void ACXInteractableGimmick::SetInteractionPromptVisible_Implementation(bool bVisible)
{
	// 판단 없이 표시만
	PromptWidget->SetVisibility(bVisible);
}


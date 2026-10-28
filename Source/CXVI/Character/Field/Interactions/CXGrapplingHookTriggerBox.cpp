// Fill out your copyright notice in the Description page of Project Settings.
#include "CXGrapplingHookTriggerBox.h"
#include "CXVI/Character/Field/CXFieldPlayerCharacter.h"
#include "CXVI/Tags/CXFieldTags.h"
#include "CXVI/UI/Field/CXFieldGrapplingUIInterface.h"

#include "AbilitySystemComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"

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

		// 박스만 숨기기.
		Collision->SetHiddenInGame(true, false);
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

	UpdateGrapplingUI(PlayerCharacter, false);

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

		UpdateGrapplingUI(PlayerCharacter, true);
	}
}

void ACXGrapplingHookTriggerBox::UpdateGrapplingUI(ACXFieldPlayerCharacter* Player, bool bCanInteract)
{
	if (!IsValid(Player) || !IsValid(GrapplingUI))
	{
		return;
	}

	APlayerController* PC =
		Cast<APlayerController>(Player->GetController());

	if (!IsValid(PC) || !PC->GetLocalPlayer())
	{
		return;
	}

	GrapplingUI->SetOwnerPlayer(PC->GetLocalPlayer());
	GrapplingUI->InitWidget();

	UUserWidget* Widget = GrapplingUI->GetUserWidgetObject();

	if (!IsValid(Widget) ||
		!Widget->GetClass()->ImplementsInterface(
			UCXFieldGrapplingUIInterface::StaticClass()))
	{
		return;
	}
	
	UCharacterMovementComponent* Movement = Player->GetCharacterMovement();

	const bool bShow =
		bCanInteract &&
		IsValid(GrapplingTarget) &&
		IsValid(LandingSurface)&&
		Movement->IsMovingOnGround();

	// 컴포넌트는 켜두고, 내부 위젯이 표시/숨김을 처리합니다.
	GrapplingUI->SetVisibility(true);

	ICXFieldGrapplingUIInterface::Execute_UpdateInteractionUI(
		Widget,
		bShow,
		bShow ? GrapplingTarget.Get() : nullptr);


}

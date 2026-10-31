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

	// 트리거 액터를 숨기면 타깃에 부착한 UI도 소유자를 따라 숨겨집니다.
	SetActorHiddenInGame(false);
	if (UShapeComponent* Collision = GetCollisionComponent())
	{
		Collision->SetHiddenInGame(true, false);
	}

	if (!IsValid(GrapplingUI) || !IsValid(GrapplingTarget))
	{
		return;
	}

	USceneComponent* TargetRoot = GrapplingTarget->GetRootComponent();
	if (!IsValid(TargetRoot))
	{
		return;
	}

	GrapplingUI->AttachToComponent(TargetRoot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	GrapplingUI->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));

	// 트리거나 타깃의 크기 때문에 UI가 확대되지 않는다.
	GrapplingUI->SetWorldScale3D(FVector::OneVector);

	GrapplingUI->InitWidget();
}

void ACXGrapplingHookTriggerBox::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ACXFieldPlayerCharacter* Player = ObservedPlayer.Get())
	{
		UpdateGrapplingUI(Player, false);
		if (UAbilitySystemComponent* ASC = ObservedASC.Get())
		{
			ASC->RemoveLooseGameplayTag(CXVIFieldTag::State_CanGrapplingHook);
		}
		if (Player->GetGrapplingTargetActor() == GrapplingTarget.Get())
		{
			Player->SetGrapplingTarget(nullptr);
		}
	}
	StopObservingUIState();
	Super::EndPlay(EndPlayReason);
}

void ACXGrapplingHookTriggerBox::ObserveUIState(ACXFieldPlayerCharacter* Player)
{
	StopObservingUIState();
	UAbilitySystemComponent* ASC = IsValid(Player) ? Player->GetAbilitySystemComponent() : nullptr;
	if (!IsValid(ASC))
	{
		return;
	}
	ObservedPlayer = Player;
	ObservedASC = ASC;
	AirborneTagHandle = ASC->RegisterGameplayTagEvent(
		CXVIFieldTag::State_Airborne, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleUIStateTagChanged);
	AbilityUITagHandle = ASC->RegisterGameplayTagEvent(
		CXVIFieldTag::State_BlockInteractionUI, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleUIStateTagChanged);
	MovingBlockTagHandle = ASC->RegisterGameplayTagEvent(
		CXVIFieldTag::State_MovingBlock, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleUIStateTagChanged);
}

void ACXGrapplingHookTriggerBox::StopObservingUIState()
{
	if (UAbilitySystemComponent* ASC = ObservedASC.Get())
	{
		ASC->RegisterGameplayTagEvent(CXVIFieldTag::State_Airborne,
			EGameplayTagEventType::NewOrRemoved).Remove(AirborneTagHandle);
		ASC->RegisterGameplayTagEvent(CXVIFieldTag::State_BlockInteractionUI,
			EGameplayTagEventType::NewOrRemoved).Remove(AbilityUITagHandle);
		ASC->RegisterGameplayTagEvent(CXVIFieldTag::State_MovingBlock,
			EGameplayTagEventType::NewOrRemoved).Remove(MovingBlockTagHandle);
	}
	ObservedPlayer.Reset();
	ObservedASC.Reset();
	AirborneTagHandle.Reset();
	AbilityUITagHandle.Reset();
	MovingBlockTagHandle.Reset();
}

void ACXGrapplingHookTriggerBox::HandleUIStateTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (ACXFieldPlayerCharacter* Player = ObservedPlayer.Get())
	{
		UpdateGrapplingUI(Player, true);
	}
}

void ACXGrapplingHookTriggerBox::HandleActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	ACXFieldPlayerCharacter* PlayerCharacter = Cast<ACXFieldPlayerCharacter>(OtherActor);

	// 유효성 검사.
	if (!IsValid(PlayerCharacter) || ObservedPlayer.Get() != PlayerCharacter)
	{
		return;
	}

	UpdateGrapplingUI(PlayerCharacter, false);
	StopObservingUIState();

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
	if (!IsValid(PlayerCharacter) || !PlayerCharacter->IsLocallyControlled() ||
		!IsValid(GrapplingTarget) || !IsValid(LandingSurface) || ObservedPlayer.Get() == PlayerCharacter)
	{
		return;
	}

	
	if (UAbilitySystemComponent* ASC = PlayerCharacter->GetAbilitySystemComponent())
	{
		// 태그 변경 보다 먼저 대상 지정.
		PlayerCharacter->SetGrapplingTarget(GrapplingTarget.Get(), LandingSurface.Get());

		ASC->AddLooseGameplayTag(CXVIFieldTag::State_CanGrapplingHook);
		ObserveUIState(PlayerCharacter);

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
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();

	const bool bShow =
		bCanInteract &&
		IsValid(GrapplingTarget) &&
		IsValid(LandingSurface) &&
		IsValid(Movement) &&
		Movement->IsMovingOnGround() &&
		IsValid(ASC) &&
		!ASC->HasMatchingGameplayTag(CXVIFieldTag::State_Airborne) &&
		!ASC->HasMatchingGameplayTag(CXVIFieldTag::State_BlockInteractionUI) &&
		!ASC->HasMatchingGameplayTag(CXVIFieldTag::State_MovingBlock);

	// 컴포넌트는 켜두고, 내부 위젯이 표시/숨김을 처리합니다.
	GrapplingUI->SetVisibility(true);

	// 표시 상태가 같으면 FadeIn을 다시 시작하지 않습니다.
	if (LastPromptWidget.Get() == Widget && bPromptShown == bShow)
	{
		return;
	}
	LastPromptWidget = Widget;
	bPromptShown = bShow;

	if (bShow || !bCanInteract)
	{
		// 표시는 위젯이 FadeIn, 범위 이탈은 위젯이 바로 숨긴다.
		ICXFieldGrapplingUIInterface::Execute_UpdateInteractionUI(
			Widget,
			bShow,
			bShow ? GrapplingTarget.Get() : nullptr);
		return;
	}

	// 범위 안에서 조건이 깨지면(그래플링 시도·점프·스프린트) 페이드 없이 바로 숨긴다.
	Widget->StopAllAnimations();
	Widget->SetVisibility(ESlateVisibility::Collapsed);
}

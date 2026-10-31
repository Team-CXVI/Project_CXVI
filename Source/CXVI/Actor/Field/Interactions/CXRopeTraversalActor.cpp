// Fill out your copyright notice in the Description page of Project Settings.


#include "CXRopeTraversalActor.h"
#include "CXVI/Character/Field/CXFieldPlayerCharacter.h"
#include "CXVI/Tags/CXFieldTags.h"
#include "CXVI/UI/Field/CXFieldGrapplingUIInterface.h"

#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

// Sets default values
ACXRopeTraversalActor::ACXRopeTraversalActor()
{
 	
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(SceneRoot);

	BottomRopePoint = CreateDefaultSubobject<USceneComponent>(TEXT("BottomRopePoint"));
	BottomRopePoint->SetupAttachment(SceneRoot);

	TopRopePoint = CreateDefaultSubobject<USceneComponent>(TEXT("TopRopePoint"));
	TopRopePoint->SetupAttachment(SceneRoot);

	BottomExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("BottomExitPoint"));
	BottomExitPoint->SetupAttachment(SceneRoot);

	TopExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("TopExitPoint"));
	TopExitPoint->SetupAttachment(SceneRoot);

	BottomInteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BottomInteractionBox"));
	BottomInteractionBox->SetupAttachment(BottomRopePoint);

	TopInteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TopInteractionBox"));
	TopInteractionBox->SetupAttachment(TopRopePoint);

	BottomInteractionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	BottomInteractionBox->SetBoxExtent(FVector(50.0f, 50.0f, 90.0f));

	BottomInteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BottomInteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	BottomInteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BottomInteractionBox->SetGenerateOverlapEvents(true);

	TopInteractionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	TopInteractionBox->SetBoxExtent(FVector(50.0f, 50.0f, 90.0f));

	TopInteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TopInteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TopInteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TopInteractionBox->SetGenerateOverlapEvents(true);

	BottomPromptUI = CreateDefaultSubobject<UWidgetComponent>(TEXT("BottomRopePrompt"));
	BottomPromptUI->SetupAttachment(BottomRopePoint);
	BottomPromptUI->SetWidgetSpace(EWidgetSpace::Screen);
	BottomPromptUI->SetDrawSize(FVector2D(100.0f, 40.0f));
	BottomPromptUI->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BottomPromptUI->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));

	TopPromptUI = CreateDefaultSubobject<UWidgetComponent>(TEXT("TopRopePrompt"));
	TopPromptUI->SetupAttachment(TopRopePoint);
	TopPromptUI->SetWidgetSpace(EWidgetSpace::Screen);
	TopPromptUI->SetDrawSize(FVector2D(100.0f, 40.0f));
	TopPromptUI->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TopPromptUI->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));
}

bool ACXRopeTraversalActor::GetTraversalPoints(ECXRopeTraversalDirection Direction, FVector& OutEntry, FVector& OutDestination, FVector& OutExit) const
{
	OutEntry = FVector::ZeroVector;
	OutDestination = FVector::ZeroVector;
	OutExit = FVector::ZeroVector;

	USceneComponent* Entry = nullptr;
	USceneComponent* Destination = nullptr;
	USceneComponent* Exit = nullptr;

	if (Direction == ECXRopeTraversalDirection::Up)
	{
		Entry = BottomRopePoint;
		Destination = TopRopePoint;
		Exit = TopExitPoint;
	}
	else if (Direction == ECXRopeTraversalDirection::Down)
	{
		Entry = TopRopePoint;
		Destination = BottomRopePoint;
		Exit = BottomExitPoint;
	}
	else
	{
		return false;
	}

	if (!IsValid(Entry) || !IsValid(Destination) || !IsValid(Exit))
	{
		return false;
	}

	OutEntry = Entry->GetComponentLocation();
	OutDestination = Destination->GetComponentLocation();
	OutExit = Exit->GetComponentLocation();

	return true;
}

bool ACXRopeTraversalActor::GetApproachPoint(ECXRopeTraversalDirection Direction, FVector& OutApproach) const
{
	USceneComponent* ApproachPoint = nullptr;
	
	if (Direction == ECXRopeTraversalDirection::Up)
	{
		ApproachPoint = BottomExitPoint.Get();
	}
	else if (Direction == ECXRopeTraversalDirection::Down)
	{
		ApproachPoint = TopExitPoint.Get();
	}
	else
	{
		return false;
	}
	
	if (!IsValid(ApproachPoint))
	{
		return false;
	}

	OutApproach = ApproachPoint->GetComponentLocation();
	return true;
}

bool ACXRopeTraversalActor::IsFacingRopePoint(const AActor* Player, ECXRopeTraversalDirection Direciton)
{
	if (!IsValid(Player))
	{
		return false;
	}

	FVector Entry, Destination, Exit;
	if (!GetTraversalPoints(Direciton, Entry, Destination, Exit))
	{
		return false;
	}

	const FVector ToPoint = (Entry - Player->GetActorLocation()).GetSafeNormal2D();

	// 포인트 바로 위에 있으면 수평 방향을 구분하지 않음.
	if (ToPoint.IsNearlyZero())
	{
		return true;
	}

	const FVector Forward = Player->GetActorForwardVector().GetSafeNormal2D();

	const float FacingDot = FVector::DotProduct(Forward, ToPoint);

	// 정면을 중심으로 좌우 45도 이내.
	return FacingDot >= 0.7071f;
}

// Called when the game starts or when spawned
void ACXRopeTraversalActor::BeginPlay()
{
	Super::BeginPlay();

	BottomInteractionBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleInteractionBeginOverlap);
	BottomInteractionBox->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::HandleInteractionEndOverlap);

	TopInteractionBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleInteractionBeginOverlap);
	TopInteractionBox->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::HandleInteractionEndOverlap);

	// 액터의 크기 때문에 UI가 확대되지 않는다.
	BottomPromptUI->SetWorldScale3D(FVector::OneVector);
	TopPromptUI->SetWorldScale3D(FVector::OneVector);

	// 컴포넌트를 처음부터 화면에 올려 두고 위젯만 접어 둔다.
	// 숨긴 컴포넌트를 켜는 프레임에 FadeIn을 재생하면 위젯이 화면에 붙기 전이라 첫 표시가 보이지 않는다.
	for (UWidgetComponent* PromptUI : { BottomPromptUI.Get(), TopPromptUI.Get() })
	{
		PromptUI->SetVisibility(true);
		PromptUI->InitWidget();
		if (UUserWidget* Widget = PromptUI->GetUserWidgetObject())
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void ACXRopeTraversalActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ACXFieldPlayerCharacter* Player = ObservedPlayer.Get())
	{
		UpdateRopeUI(Player, false);
	}
	RopePromptUI.Reset();
	StopObservingUIState();
	Super::EndPlay(EndPlayReason);
}

void ACXRopeTraversalActor::HandleInteractionBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(OtherActor);

	if (!IsValid(Player) || OtherComp != Player->GetCapsuleComponent())
	{
		return;
	}

	const ECXRopeTraversalDirection Direction = OverlappedComponent == BottomInteractionBox.Get() ? ECXRopeTraversalDirection::Up : ECXRopeTraversalDirection::Down;
	Player->SetRopeTraversalTarget(this, Direction);

	const TCHAR* Side = OverlappedComponent == BottomInteractionBox.Get() ? TEXT("하단") : TEXT("상단");

	UE_LOG(LogTemp, Log, TEXT("[Rope] %s 진입: %s"), Side, *GetNameSafe(Player));

	if (!Player->IsLocallyControlled())
	{
		return;
	}

	UWidgetComponent* PromptUI = OverlappedComponent == BottomInteractionBox.Get() ? BottomPromptUI.Get() : TopPromptUI.Get();

	// 반대쪽 UI가 켜져 있으면 먼저 끈다.
	if (RopePromptUI.IsValid() && RopePromptUI.Get() != PromptUI)
	{
		UpdateRopeUI(Player, false);
	}

	RopePromptUI = PromptUI;
	ObserveUIState(Player);
	UpdateRopeUI(Player, true);
}

void ACXRopeTraversalActor::HandleInteractionEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(OtherActor);

	if (!IsValid(Player) || OtherComp != Player->GetCapsuleComponent())
	{
		return;
	}

	const ECXRopeTraversalDirection Direction = OverlappedComponent == BottomInteractionBox.Get() ? ECXRopeTraversalDirection::Up : ECXRopeTraversalDirection::Down;
	Player->ClearRopeTraversalTarget(this, Direction);

	const TCHAR* Side = OverlappedComponent == BottomInteractionBox.Get() ? TEXT("하단") : TEXT("상단");

	UE_LOG(LogTemp, Log, TEXT("[Rope] %s 이탈: %s"), Side, *GetNameSafe(Player));

	UWidgetComponent* PromptUI = OverlappedComponent == BottomInteractionBox.Get() ? BottomPromptUI.Get() : TopPromptUI.Get();

	// 현재 표시 중인 쪽에서 나갈 때만 UI를 끈다.
	if (ObservedPlayer.Get() != Player || RopePromptUI.Get() != PromptUI)
	{
		return;
	}

	UpdateRopeUI(Player, false);
	RopePromptUI.Reset();
	StopObservingUIState();
}

void ACXRopeTraversalActor::UpdateRopeUI(ACXFieldPlayerCharacter* Player, bool bInRange)
{
	UWidgetComponent* PromptUI = RopePromptUI.Get();

	if (!IsValid(Player) || !IsValid(PromptUI))
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Player->GetController());

	if (!IsValid(PC) || !PC->GetLocalPlayer())
	{
		return;
	}

	PromptUI->SetOwnerPlayer(PC->GetLocalPlayer());
	PromptUI->InitWidget();

	UUserWidget* Widget = PromptUI->GetUserWidgetObject();

	if (!IsValid(Widget) ||
		!Widget->GetClass()->ImplementsInterface(
			UCXFieldGrapplingUIInterface::StaticClass()))
	{
		return;
	}

	const bool bShow = bInRange && CanShowRopeUI(Player);

	// 표시 상태가 같으면 FadeIn을 다시 시작하지 않습니다.
	if (LastPromptWidget.Get() == Widget && bPromptShown == bShow)
	{
		return;
	}
	LastPromptWidget = Widget;
	bPromptShown = bShow;

	if (bShow || !bInRange)
	{
		// 표시는 위젯이 FadeIn, 범위 이탈은 위젯이 바로 숨긴다.
		ICXFieldGrapplingUIInterface::Execute_UpdateInteractionUI(
			Widget,
			bShow,
			bShow ? this : nullptr);
		return;
	}

	// 범위 안에서 조건이 깨지면(점프·스프린트 등 어빌리티·로프 사용 중·시선 이탈) 페이드 없이 바로 숨긴다.
	Widget->StopAllAnimations();
	Widget->SetVisibility(ESlateVisibility::Collapsed);
}

bool ACXRopeTraversalActor::CanShowRopeUI(const ACXFieldPlayerCharacter* Player)
{
	const UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
	const UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	if (!IsValid(Movement) || !Movement->IsMovingOnGround() || !IsValid(ASC))
	{
		return false;
	}

	// 공통 조건(그래플링과 동일): 점프·낙하, 어빌리티 사용 중(스프린트·로프 이동 등), 이동 제한.
	if (ASC->HasMatchingGameplayTag(CXVIFieldTag::State_Airborne) ||
		ASC->HasMatchingGameplayTag(CXVIFieldTag::State_BlockInteractionUI) ||
		ASC->HasMatchingGameplayTag(CXVIFieldTag::State_MovingBlock))
	{
		return false;
	}

	// 로프 고유 조건: 로프 사용 조건과 같은 기준(정면 45도 안의 로프 포인트)으로 시선을 확인한다.
	const ECXRopeTraversalDirection Direction =
		RopePromptUI.Get() == BottomPromptUI.Get() ? ECXRopeTraversalDirection::Up : ECXRopeTraversalDirection::Down;
	return IsFacingRopePoint(Player, Direction);
}

void ACXRopeTraversalActor::ObserveUIState(ACXFieldPlayerCharacter* Player)
{
	StopObservingUIState();
	UAbilitySystemComponent* ASC = IsValid(Player) ? Player->GetAbilitySystemComponent() : nullptr;
	if (!IsValid(ASC))
	{
		return;
	}
	ObservedPlayer = Player;
	ObservedASC = ASC;

	// 공통 조건은 그래플링과 같은 태그 이벤트로 감지한다.
	AirborneTagHandle = ASC->RegisterGameplayTagEvent(
		CXVIFieldTag::State_Airborne, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleUIStateTagChanged);
	AbilityUITagHandle = ASC->RegisterGameplayTagEvent(
		CXVIFieldTag::State_BlockInteractionUI, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleUIStateTagChanged);
	MovingBlockTagHandle = ASC->RegisterGameplayTagEvent(
		CXVIFieldTag::State_MovingBlock, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleUIStateTagChanged);

	// 시선은 이벤트가 없어서 주기적으로 확인한다.
	GetWorldTimerManager().SetTimer(
		UIRefreshTimerHandle, this, &ThisClass::RefreshRopeUI, 0.05f, true);
}

void ACXRopeTraversalActor::StopObservingUIState()
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
	GetWorldTimerManager().ClearTimer(UIRefreshTimerHandle);
	ObservedPlayer.Reset();
	ObservedASC.Reset();
	AirborneTagHandle.Reset();
	AbilityUITagHandle.Reset();
	MovingBlockTagHandle.Reset();
}

void ACXRopeTraversalActor::HandleUIStateTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (ACXFieldPlayerCharacter* Player = ObservedPlayer.Get())
	{
		UpdateRopeUI(Player, true);
	}
}

void ACXRopeTraversalActor::RefreshRopeUI()
{
	if (ACXFieldPlayerCharacter* Player = ObservedPlayer.Get())
	{
		UpdateRopeUI(Player, true);
	}
}

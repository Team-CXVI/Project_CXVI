// Fill out your copyright notice in the Description page of Project Settings.


#include "CXFieldItemPickup.h"
#include "CXVI/Character/Field/CXFieldPlayerCharacter.h"
#include "CXVI/Foundation/UserData/CXUserDataSubsystem.h"
#include "CXVI/Tags/CXFieldTags.h"

#include "Engine/World.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldItemPickup)

// Sets default values
ACXFieldItemPickup::ACXFieldItemPickup()
{
	USceneComponent* SceneRoot =
		CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

	SetRootComponent(SceneRoot);

	ItemMesh =
		CreateDefaultSubobject<UStaticMeshComponent>(
			TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(SceneRoot);

	// 표시 용도. 플레이어 이동을 막지 않도록 충돌 제거.
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ItemPickupInteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ItemPickupInteractionSphere"));
	ItemPickupInteractionSphere->SetupAttachment(SceneRoot);

	ItemPickupInteractionSphere->SetRelativeLocation(FVector::ZeroVector);
	ItemPickupInteractionSphere->SetSphereRadius(100.0f);
	ItemPickupInteractionSphere->SetGenerateOverlapEvents(true);

	ItemPickupPromptUI =
		CreateDefaultSubobject<UWidgetComponent>(TEXT("ItemPickupPrompt"));

	ItemPickupPromptUI->SetupAttachment(SceneRoot);
	ItemPickupPromptUI->SetWidgetSpace(EWidgetSpace::Screen);
	ItemPickupPromptUI->SetDrawSize(FVector2D(48.0f, 48.0f));
	ItemPickupPromptUI->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACXFieldItemPickup::InitializePickup(FName InSpawnId, const TArray<FCXItemStack>& InRewards)
{
	SpawnId = InSpawnId;
	Rewards = InRewards;
}

bool ACXFieldItemPickup::CanPickup(const ACXFieldPlayerCharacter* Player) const
{
	if (!IsValid(Player) || IsActorBeingDestroyed())
	{
		return false;
	}

	return CanShowItemUI(Player);
}

void ACXFieldItemPickup::Pickup(ACXFieldPlayerCharacter* Player)
{
	if (!CanPickup(Player))
	{
		return;
	}

	UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this);
	if (!IsValid(UserData))
	{
		return;
	}

	for (const FCXItemStack& Reward : Rewards)
	{
		UserData->AddItem(Reward.ItemId, Reward.Count);
	}

	if (SpawnId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: SpawnId가 없어 획득을 기록할 수 없습니다."), *GetName());
	}
	else
	{
		UserData->MarkItemCollected(SpawnId);
	}

	// 태그·등록·UI 정리는 EndPlay에서 처리.
	Destroy();
}

// Called when the game starts or when spawned
void ACXFieldItemPickup::BeginPlay()
{
	Super::BeginPlay();

	ItemPickupInteractionSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleInteractionBeginOverlap);
	ItemPickupInteractionSphere->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::HandleInteractionEndOverlap);

	ItemPickupPromptUI->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));

	// 아이템 크기 때문에 UI가 확대되지 않는다.
	ItemPickupPromptUI->SetWorldScale3D(FVector::OneVector);

	ItemPickupPromptUI->SetVisibility(false);

	// 세이브 로드 등으로 아이템 범위 안에서 시작하면, 플레이어가 BeginPlay보다 먼저 스폰돼 진입 이벤트를 놓친다.
	// 한 틱 뒤(빙의·ASC 초기화 이후) 이미 겹쳐 있는 플레이어를 진입 처리한다.
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		TArray<UPrimitiveComponent*> OverlappingComponents;
		ItemPickupInteractionSphere->GetOverlappingComponents(OverlappingComponents);
		for (UPrimitiveComponent* OtherComp : OverlappingComponents)
		{
			HandleInteractionBeginOverlap(ItemPickupInteractionSphere, OtherComp->GetOwner(), OtherComp, 0, false, FHitResult());
		}
	}));
}

void ACXFieldItemPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 깃발과 달리 플레이어가 범위 안에 있는 채로 제거되므로, EndOverlap이 하던 정리를 여기서 한다.
	if (ACXFieldPlayerCharacter* Player = ObservedPlayer.Get())
	{
		if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
		{
			ASC->RemoveLooseGameplayTag(CXVIFieldTag::State_CanPickupItem);
		}

		Player->UnregisterNearByItem(this);
		UpdateItemUI(Player, false);
	}

	StopObservingUIState();
	Super::EndPlay(EndPlayReason);
}

void ACXFieldItemPickup::HandleInteractionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(OtherActor);

	// 플레이어의 메시 등 다른 컴포넌트 오버랩은 무시한다. 같은 플레이어의 중복 진입도 무시한다.
	if (!IsValid(Player) || OtherComp != Player->GetCapsuleComponent() || ObservedPlayer.Get() == Player)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
	{
		ASC->AddLooseGameplayTag(CXVIFieldTag::State_CanPickupItem);
	}

	Player->RegisterNearByItem(this);
	ObserveUIState(Player);
	UpdateItemUI(Player, true);
}

void ACXFieldItemPickup::HandleInteractionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(OtherActor);

	if (!IsValid(Player) || OtherComp != Player->GetCapsuleComponent() || ObservedPlayer.Get() != Player)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
	{
		ASC->RemoveLooseGameplayTag(CXVIFieldTag::State_CanPickupItem);
	}

	Player->UnregisterNearByItem(this);
	UpdateItemUI(Player, false);
	StopObservingUIState();
}

void ACXFieldItemPickup::UpdateItemUI(ACXFieldPlayerCharacter* Player, bool bInRange)
{
	if (!IsValid(Player) || !IsValid(ItemPickupPromptUI))
	{
		return;
	}

	const bool bShow = bInRange && CanShowItemUI(Player);
	ItemPickupPromptUI->SetVisibility(bShow);
}

bool ACXFieldItemPickup::CanShowItemUI(const ACXFieldPlayerCharacter* Player) const
{
	const UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
	const UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	if (!IsValid(Movement) || !Movement->IsMovingOnGround() || !IsValid(ASC))
	{
		return false;
	}

	// 공통 조건(깃발·그래플링과 동일): 점프·낙하, 어빌리티 사용 중(스프린트 등), 이동 제한.
	if (ASC->HasMatchingGameplayTag(CXVIFieldTag::State_Airborne) ||
		ASC->HasMatchingGameplayTag(CXVIFieldTag::State_BlockInteractionUI) ||
		ASC->HasMatchingGameplayTag(CXVIFieldTag::State_MovingBlock))
	{
		return false;
	}

	// 메뉴가 열려 있는 동안은 숨긴다.
	if (ASC->HasMatchingGameplayTag(CXVIFieldTag::State_InMenu))
	{
		return false;
	}

	// 플레이어 정면 기준 PromptFacingAngle 안에 아이템이 있어야 한다.
	const FVector ToItem = (GetActorLocation() - Player->GetActorLocation()).GetSafeNormal2D();
	if (ToItem.IsNearlyZero())
	{
		return true;
	}
	const FVector Forward = Player->GetActorForwardVector().GetSafeNormal2D();
	return FVector::DotProduct(Forward, ToItem) >= FMath::Cos(FMath::DegreesToRadians(PromptFacingAngle));
}

void ACXFieldItemPickup::ObserveUIState(ACXFieldPlayerCharacter* Player)
{
	StopObservingUIState();
	UAbilitySystemComponent* ASC = IsValid(Player) ? Player->GetAbilitySystemComponent() : nullptr;
	if (!IsValid(ASC))
	{
		return;
	}
	ObservedPlayer = Player;
	ObservedASC = ASC;

	// 공통 조건은 깃발·그래플링과 같은 태그 이벤트로 감지한다.
	AirborneTagHandle = ASC->RegisterGameplayTagEvent(
		CXVIFieldTag::State_Airborne, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleUIStateTagChanged);
	AbilityUITagHandle = ASC->RegisterGameplayTagEvent(
		CXVIFieldTag::State_BlockInteractionUI, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleUIStateTagChanged);
	MovingBlockTagHandle = ASC->RegisterGameplayTagEvent(
		CXVIFieldTag::State_MovingBlock, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleUIStateTagChanged);
	InMenuTagHandle = ASC->RegisterGameplayTagEvent(
		CXVIFieldTag::State_InMenu, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleUIStateTagChanged);

	// 시선은 이벤트가 없어서 주기적으로 확인한다.
	GetWorldTimerManager().SetTimer(
		UIRefreshTimerHandle, this, &ThisClass::RefreshItemUI, 0.05f, true);
}

void ACXFieldItemPickup::StopObservingUIState()
{
	if (UAbilitySystemComponent* ASC = ObservedASC.Get())
	{
		ASC->RegisterGameplayTagEvent(CXVIFieldTag::State_Airborne,
			EGameplayTagEventType::NewOrRemoved).Remove(AirborneTagHandle);
		ASC->RegisterGameplayTagEvent(CXVIFieldTag::State_BlockInteractionUI,
			EGameplayTagEventType::NewOrRemoved).Remove(AbilityUITagHandle);
		ASC->RegisterGameplayTagEvent(CXVIFieldTag::State_MovingBlock,
			EGameplayTagEventType::NewOrRemoved).Remove(MovingBlockTagHandle);
		ASC->RegisterGameplayTagEvent(CXVIFieldTag::State_InMenu,
			EGameplayTagEventType::NewOrRemoved).Remove(InMenuTagHandle);
	}
	GetWorldTimerManager().ClearTimer(UIRefreshTimerHandle);
	ObservedPlayer.Reset();
	ObservedASC.Reset();
	AirborneTagHandle.Reset();
	AbilityUITagHandle.Reset();
	MovingBlockTagHandle.Reset();
	InMenuTagHandle.Reset();
}

void ACXFieldItemPickup::HandleUIStateTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (ACXFieldPlayerCharacter* Player = ObservedPlayer.Get())
	{
		UpdateItemUI(Player, true);
	}
}

void ACXFieldItemPickup::RefreshItemUI()
{
	if (ACXFieldPlayerCharacter* Player = ObservedPlayer.Get())
	{
		UpdateItemUI(Player, true);
	}
}

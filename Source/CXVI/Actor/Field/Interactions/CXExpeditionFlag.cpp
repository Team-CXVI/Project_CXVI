// Fill out your copyright notice in the Description page of Project Settings.


#include "CXExpeditionFlag.h"
#include "CXVI/Foundation/Field/FieldEnemySpawning/CXFieldEnemySubsystem.h"
#include "CXVI/Character/Field/CXFieldPlayerCharacter.h"
#include "CXVI/Tags/CXFieldTags.h"

#include "Engine/World.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXExpeditionFlag)

// Sets default values
ACXExpeditionFlag::ACXExpeditionFlag()
{
    USceneComponent* SceneRoot =
        CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

    SetRootComponent(SceneRoot);

    ExpeditionFlag =
        CreateDefaultSubobject<UStaticMeshComponent>(
            TEXT("ExpeditionFlag"));
    ExpeditionFlag->SetupAttachment(SceneRoot);

    // Keep the serialized subobject name so existing Blueprint overrides remain valid.
    //ExpeditionFlagTrigger =
    //    CreateDefaultSubobject<UChildActorComponent>(
    //        TEXT("SaveFlagTrigger"));
    //ExpeditionFlagTrigger->SetupAttachment(SceneRoot);
    //ExpeditionFlagTrigger->SetChildActorClass(
    //    ACXExpeditionFlagTriggerBox::StaticClass());
    //ExpeditionFlagTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));

    ExpeditionInteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ExpeditionInteractionSphere"));
    ExpeditionInteractionSphere->SetupAttachment(SceneRoot);

    ExpeditionInteractionSphere->SetRelativeLocation(FVector::ZeroVector);
    ExpeditionInteractionSphere->SetSphereRadius(100.0f);
    ExpeditionInteractionSphere->SetGenerateOverlapEvents(true);

    FlagPromptUI =
        CreateDefaultSubobject<UWidgetComponent>(TEXT("FlagPrompt"));

    FlagPromptUI->SetupAttachment(GetRootComponent());
    FlagPromptUI->SetWidgetSpace(EWidgetSpace::Screen);
    FlagPromptUI->SetDrawSize(FVector2D(48.0f, 48.0f));
}

bool ACXExpeditionFlag::CanFlag(const ACXFieldPlayerCharacter* Player) const
{
    if (!IsValid(Player) || !IsValid(ExpeditionInteractionSphere))
    {
        return false;
    }

    return true;
}

void ACXExpeditionFlag::CompleteFlag()
{
    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        return;
    }

    UCXFieldEnemySubsystem* EnemySubsystem = World->GetSubsystem<UCXFieldEnemySubsystem>();
    if (!IsValid(EnemySubsystem))
    {
        return;
    }
    const int32 RespawnedCount = EnemySubsystem->RespawnDefeatedEnemies(NAME_None);

    UE_LOG(LogTemp, Log, TEXT("깃발 휴식: 필드 적 %d 마리 복원"), RespawnedCount);
}

// Called when the game starts or when spawned
void ACXExpeditionFlag::BeginPlay()
{
	Super::BeginPlay();
	
    ExpeditionInteractionSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleInteractionBeginOverlap);
    ExpeditionInteractionSphere->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::HandleInteractionEndOverlap);

    FlagPromptUI->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);

    FlagPromptUI->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));

    // 트리거나 타깃의 크기 때문에 UI가 확대되지 않는다.
    FlagPromptUI->SetWorldScale3D(FVector::OneVector);

    FlagPromptUI->SetVisibility(false);

    // 세이브 로드 등으로 깃발 범위 안에서 시작하면, 플레이어가 BeginPlay보다 먼저 스폰돼 진입 이벤트를 놓친다.
    // 한 틱 뒤(빙의·ASC 초기화 이후) 이미 겹쳐 있는 플레이어를 진입 처리한다.
    GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
    {
        TArray<UPrimitiveComponent*> OverlappingComponents;
        ExpeditionInteractionSphere->GetOverlappingComponents(OverlappingComponents);
        for (UPrimitiveComponent* OtherComp : OverlappingComponents)
        {
            HandleInteractionBeginOverlap(ExpeditionInteractionSphere, OtherComp->GetOwner(), OtherComp, 0, false, FHitResult());
        }
    }));
}

void ACXExpeditionFlag::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ACXFieldPlayerCharacter* Player = ObservedPlayer.Get())
    {
        UpdateFlagUI(Player, false);
    }

    StopObservingUIState();
    Super::EndPlay(EndPlayReason);
}

void ACXExpeditionFlag::HandleInteractionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(OtherActor);

    // 플레이어의 메시 등 다른 컴포넌트 오버랩은 무시한다. 같은 플레이어의 중복 진입도 무시한다.
    if (!IsValid(Player) || OtherComp != Player->GetCapsuleComponent() || ObservedPlayer.Get() == Player)
    {
        return;
    }

    if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
    {
        // 그래플링 훅 처럼해당 타겟에 UI.

        ASC->AddLooseGameplayTag(CXVIFieldTag::State_CanUseExpeditionFlag);
    }

    ACXExpeditionFlag* Flag = this;
    if (!IsValid(Flag))
    {
        return;
    }

    Player->RegisterNearByFlag(Flag);
    ObserveUIState(Player);
    UpdateFlagUI(Player, true);
}

void ACXExpeditionFlag::HandleInteractionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(OtherActor);

    if (!IsValid(Player) || OtherComp != Player->GetCapsuleComponent() || ObservedPlayer.Get() != Player)
    {
        return;
    }

    if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
    {
        // 그래플링 훅 처럼해당 타겟에 UI.

        ASC->RemoveLooseGameplayTag(CXVIFieldTag::State_CanUseExpeditionFlag);

        // 임시: 메뉴가 생기기 전까지는 깃발 범위를 벗어날 때 메뉴 태그를 뗀다.
        ASC->SetLooseGameplayTagCount(CXVIFieldTag::State_InMenu, 0);
    }

    ACXExpeditionFlag* Flag = this;
    if (!IsValid(Flag))
    {
        return;
    }

    Player->UnregisterNearByFlag(Flag);
    UpdateFlagUI(Player, false);
    StopObservingUIState();
}

void ACXExpeditionFlag::UpdateFlagUI(ACXFieldPlayerCharacter* Player, bool bInRange)
{
    if (!IsValid(Player) || !IsValid(FlagPromptUI))
    {
        return;
    }


    const bool bShow = bInRange && CanShowFlagUI(Player);
	FlagPromptUI->SetVisibility(bShow);
}

bool ACXExpeditionFlag::CanShowFlagUI(const ACXFieldPlayerCharacter* Player) const
{
    const UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
    const UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
    if (!IsValid(Movement) || !Movement->IsMovingOnGround() || !IsValid(ASC))
    {
        return false;
    }

    // 공통 조건(그래플링과 동일): 점프·낙하, 어빌리티 사용 중(스프린트 등), 이동 제한.
    if (ASC->HasMatchingGameplayTag(CXVIFieldTag::State_Airborne) ||
        ASC->HasMatchingGameplayTag(CXVIFieldTag::State_BlockInteractionUI) ||
        ASC->HasMatchingGameplayTag(CXVIFieldTag::State_MovingBlock))
    {
        return false;
    }

    // 깃발 고유 조건: 깃발 메뉴가 열려 있는 동안은 숨긴다.
    if (ASC->HasMatchingGameplayTag(CXVIFieldTag::State_InMenu))
    {
        return false;
    }

    // 깃발 고유 조건: 플레이어 정면 기준 PromptFacingAngle 안에 깃발이 있어야 한다.
    const FVector ToFlag = (GetActorLocation() - Player->GetActorLocation()).GetSafeNormal2D();
    if (ToFlag.IsNearlyZero())
    {
        return true;
    }
    const FVector Forward = Player->GetActorForwardVector().GetSafeNormal2D();
    return FVector::DotProduct(Forward, ToFlag) >= FMath::Cos(FMath::DegreesToRadians(PromptFacingAngle));
}

void ACXExpeditionFlag::ObserveUIState(ACXFieldPlayerCharacter* Player)
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
    InMenuTagHandle = ASC->RegisterGameplayTagEvent(
        CXVIFieldTag::State_InMenu, EGameplayTagEventType::NewOrRemoved)
        .AddUObject(this, &ThisClass::HandleUIStateTagChanged);

    // 시선은 이벤트가 없어서 주기적으로 확인한다.
    GetWorldTimerManager().SetTimer(
        UIRefreshTimerHandle, this, &ThisClass::RefreshFlagUI, 0.05f, true);
}

void ACXExpeditionFlag::StopObservingUIState()
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

void ACXExpeditionFlag::HandleUIStateTagChanged(const FGameplayTag Tag, int32 NewCount)
{
    if (ACXFieldPlayerCharacter* Player = ObservedPlayer.Get())
    {
        UpdateFlagUI(Player, true);
    }
}

void ACXExpeditionFlag::RefreshFlagUI()
{
    if (ACXFieldPlayerCharacter* Player = ObservedPlayer.Get())
    {
        UpdateFlagUI(Player, true);
    }
}

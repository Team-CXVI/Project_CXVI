// Fill out your copyright notice in the Description page of Project Settings.


#include "CXExpeditionFlag.h"
#include "CXVI/Foundation/Field/FieldEnemySpawning/CXFieldEnemySubsystem.h"
#include "CXVI/Character/Field/CXFieldPlayerCharacter.h"
#include "CXVI/Tags/CXFieldTags.h"
#include "CXVI/UI/Field/CXFieldGrapplingUIInterface.h"

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
    FlagPromptUI->SetDrawSize(FVector2D(100.0f, 40.0f));
    FlagPromptUI->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool ACXExpeditionFlag::CanFlag(const ACXFieldPlayerCharacter* Player) const
{
    if (!IsValid(Player) || !IsValid(ExpeditionInteractionSphere))
    {
        return false;
    }
    
    if (!CanShowFlagUI(Player))
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

    // 컴포넌트를 처음부터 화면에 올려 두고 위젯만 접어 둔다.
    // 숨긴 컴포넌트를 켜는 프레임에 FadeIn을 재생하면 위젯이 화면에 붙기 전이라 첫 표시가 보이지 않는다.
    FlagPromptUI->SetVisibility(true);
    FlagPromptUI->InitWidget();
    if (UUserWidget* Widget = FlagPromptUI->GetUserWidgetObject())
    {
        Widget->SetVisibility(ESlateVisibility::Collapsed);
    }

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

    FlagPromptUI->InitWidget();

    UUserWidget* Widget = FlagPromptUI->GetUserWidgetObject();

    if (!IsValid(Widget) || !Widget->GetClass()->ImplementsInterface(
        UCXFieldGrapplingUIInterface::StaticClass()
    ))
    {
        return;
    }

    const bool bShow = bInRange && CanShowFlagUI(Player);

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

    // 범위 안에서 조건이 깨지면(점프·스프린트 등 어빌리티·메뉴 열림·시선 이탈) 페이드 없이 바로 숨긴다.
    Widget->StopAllAnimations();
    Widget->SetVisibility(ESlateVisibility::Collapsed);
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

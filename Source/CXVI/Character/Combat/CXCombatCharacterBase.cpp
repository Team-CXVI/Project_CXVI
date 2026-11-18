// Fill out your copyright notice in the Description page of Project Settings.


#include "CXCombatCharacterBase.h"

#include "CXVI/Tags/CXGameplayTags.h"

#include "CXVI/AbilitySystem/Attributes/Combat/CXCombatAttributeSetBase.h"
#include "AbilitySystem/Abilities/Combat/CXGameplayAbilityCombatBase.h"
#include "AbilitySystem/Abilities/Combat/CXAbilityData.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"
#include "AbilitySystem/Abilities/Combat/CXGA_Dodge.h"
#include "Actor/Combat/CXCombatArena.h"
#include "Foundation/Combat/CXCombatManagerSubsystem.h"

#include "AI/Combat/CXCombatAIController.h"

#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
//#include "GameplayAbilitySpec.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatCharacterBase)

// Sets default values
ACXCombatCharacterBase::ACXCombatCharacterBase()
{
	// Pawn
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	//GetCapsuleComponent()->SetCollisionProfileName(TEXT(""));

	TargetFocusPoint = CreateDefaultSubobject<USceneComponent>(TEXT("TargetFocusPoint"));
	TargetFocusPoint->SetupAttachment(RootComponent);
	TargetFocusPoint->SetRelativeLocation(FVector(0.f, 0.f, 44.f));
	TargetFocusPoint->ComponentTags.AddUnique(FName(TEXT("TargetSelectFocus")));

	// Movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 1200.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;

	// Mesh
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -100.0f), FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	GetMesh()->SetCollisionProfileName(TEXT("NoCollision"));
	// 라이팅 채널 : 0(기본 라이트) + 1(아군 전용 라이트, 아레나의 DL_*_PlayerKey). 몬스터는 ACXCombatCharacterEnemy에서 0 + 2로 덮어씀
	GetMesh()->SetLightingChannels(true, true, false);

	// Widget
	HpBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("HpBar"));
	HpBar->SetupAttachment(GetMesh());
	HpBar->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	HpBar->SetWidgetSpace(EWidgetSpace::Screen);
	HpBar->SetDrawSize(FVector2D(180.f, 20.f));
	HpBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Damage UI Widget Anchor
	DamageNumberAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("DamageNumberAnchor"));
	DamageNumberAnchor->SetupAttachment(RootComponent);
	DamageNumberAnchor->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
	DamageNumberAnchor->ComponentTags.AddUnique(FName(TEXT("DamageNumberAnchor")));

	// ASC
	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));

	// AttributeSet
	//CharacterAttributeSet = CreateDefaultSubobject<UCXCombatAttributeSet>(TEXT("CharacterAttributeSet"));
	//ASC->AddAttributeSetSubobject<UCXCombatAttributeSet>(CharacterAttributeSet);

	// 캐릭터 기본변수 초기화
	bIsCharacterAlive = true;
	bIsTurn = false;

	AIControllerClass = ACXCombatAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

bool ACXCombatCharacterBase::CanSelectedAttackBeParried() const
{
	const FGameplayAbilitySpec* Spec = GetSelectedAttackSpec();

	const UCXGameplayAbilityCombatBase* AttackAbility = Spec
		? Cast<UCXGameplayAbilityCombatBase>(Spec->Ability)
		: nullptr;

	return !AttackAbility || AttackAbility->GetAttackTargetMode() != ECXAttackTargetMode::AllOpponents;
}

// Called when the game starts or when spawned
void ACXCombatCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	InitializeGAS();
	
}

void ACXCombatCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (!IsValid(ASC) || !IsValid(CharacterAttributeSet))
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 초기화 실패, ASC=%s, Attribute=%s"), *GetName(), *GetNameSafe(ASC), *GetNameSafe(CharacterAttributeSet));
		return;
	}

	CXPostInitializeComponents();

	CharacterAttributeSet->OnOutOfHealth.AddUniqueDynamic(this, &ACXCombatCharacterBase::OnOutOfHealthCpp);


	CharacterAttributeSet->OnDamageApplied.AddUObject(this, &ACXCombatCharacterBase::HandleDamageApplied);
	CharacterAttributeSet->OnParrySucceeded.AddUObject(this, &ACXCombatCharacterBase::HandleParrySucceeded);
	CharacterAttributeSet->OnDodgeSucceeded.AddUObject(this, &ACXCombatCharacterBase::HandleDodgeSucceeded);
}

void ACXCombatCharacterBase::InitializeGAS()
{
	if (!IsValid(ASC))
	{
		return;
	}

	// 빙의 후 컨트롤러 정보가 달라질 수 있으므로 다시 갱신합니다.
	ASC->InitAbilityActorInfo(this, this);

	// 어빌리티 부여는 서버에서 한 번만 처리합니다.
	if (!HasAuthority() || bIsInitializedAbility)
	{
		return;
	}

	bIsInitializedAbility = true;

	for (const auto& Ability : Abilities)
	{
		if (Ability)
		{
			ASC->GiveAbility(FGameplayAbilitySpec(Ability));
		}
		
	}
}

void ACXCombatCharacterBase::NotifyBattleAbilityEnded(FGameplayAbilitySpecHandle Handle, bool bWasCancelled)
{
	CurrentActivatingHandles.Remove(Handle);

	if (Handle != ExecutingCombatAbility)
	{
		return;
	}

	ExecutingCombatAbility = FGameplayAbilitySpecHandle();

	// 패링 가능한 상태
	if (bCounterAttackPending)
	{
		bCounterAttackPending = false;

		PendingAction.Clear();
		PendingAction.Type = ECXActionType::CounterAttack;

		CurrentTargetPawn = CounterAttackTarget.Get();

		if (ActionPhase())
		{
			return;

		}

		UE_LOG(LogTemp, Warning, TEXT("%s : 반격 요청 실패"), *GetName());

	}

	bDoingAction = false;
	UE_LOG(LogTemp, Display, TEXT("%s : 행동 종료"), *GetName());



	// BT에 공격 종료 알림
	OnCombatAbilityFinished.Broadcast(bWasCancelled);

	// 턴매니저에게도 공격 종료 알림
	FinishAction();
}

void ACXCombatCharacterBase::NotifyAttackStarted(int32 TotalHitCount)
{
	FCXCombatMessageData Message;
	Message.Character = this;
	Message.Phase = ECXCombatPhase::ActionPhase;
	Message.MessageType = ECXCombatMessageType::AttackStarted;
	Message.TotalHitCount = TotalHitCount;

	CombatMessageResponse(Message);
}

void ACXCombatCharacterBase::FinishAction()
{
	/*if ( SavedMessage.Phase != ECXCombatPhase::ActionPhase)
	{
		return;
	}*/
	
	FCXCombatMessageData Response = SavedMessage;
	Response.Character = this;
	Response.MessageType = ECXCombatMessageType::Action;

	CombatMessageResponse(Response);
}

UAbilitySystemComponent* ACXCombatCharacterBase::GetAbilitySystemComponent() const
{
	return ASC;
}

void ACXCombatCharacterBase::OnOutOfHealthCpp(AActor* InInstigator)
{
	if (!bIsCharacterAlive)
	{
		return;
	}

	HpBar->SetVisibility(false);

	bIsCharacterAlive = false;
	if (HasActorBegunPlay())
	{
		PlayDeathPresentation();
	}
}

void ACXCombatCharacterBase::HandleDamageApplied(const FCXDamageAppliedData& InDamageData)
{
	if (InDamageData.Amount <= 0.f)
	{
		return;
	}
	NextDefenseAllowedTime = 0.f;

	BroadcastDamageResult(InDamageData.Instigator, InDamageData.bIsCritical ? ECXDamageResult::Critical : ECXDamageResult::Hit, InDamageData.Amount);

	if (CharacterAttributeSet->GetHealth() > 0.f)
	{
		StartHitReactMontage();
	}

	ACXCombatCharacterBase* Attacker = Cast<ACXCombatCharacterBase>(InDamageData.Instigator);

	// this는 본인이 치고 본인이 맞은 경우는 제거
	if (!IsValid(Attacker) || Attacker == this)
	{
		return;
	}

	Attacker->TryGainBasicAttackMana();
}

void ACXCombatCharacterBase::HandleParrySucceeded(AActor* InInstigator)
{
	if (!HasAuthority() || !IsValid(InInstigator))
	{
		return;
	}

	NextDefenseAllowedTime = 0.f;

	ACXCombatCharacterBase* Attacker = Cast<ACXCombatCharacterBase>(InInstigator);
	if (Attacker != CounterAttackTarget)
	{
		CounterAttackTarget = Attacker;
	}

	FCXCombatMessageData Message;
	Message.Character = this;
	Message.Attacker = Cast<APawn>(InInstigator);
	Message.Phase = ECXCombatPhase::ActionPhase;
	Message.MessageType = ECXCombatMessageType::ParrySucceeded;

	CombatMessageResponse(Message);

	const FGameplayAbilitySpec* Spec =
		ASC->FindAbilitySpecFromHandle(ExecutingCombatAbility);

	if (Spec && Spec->Ability &&
		Spec->Ability->GetAssetTags().HasTagExact(CXVIGameplayTags::Ability_Parry))
	{
		if (UCXGA_Dodge* ParryAbility = Cast<UCXGA_Dodge>(Spec->GetPrimaryInstance()))
		{
			ParryAbility->FinishSuccessfulReaction();
		}
	}

	BroadcastDamageResult(InInstigator, ECXDamageResult::Parried, 0.f);
}

void ACXCombatCharacterBase::HandleDodgeSucceeded(AActor* InInstigator)
{
	if (HasAuthority() && IsValid(ASC))
	{
		const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(ExecutingCombatAbility);

		if (Spec && Spec->Ability && Spec->IsActive() &&
			Spec->Ability->GetAssetTags().HasTagExact(CXVIGameplayTags::Ability_Dodge) &&
			!Spec->Ability->GetAssetTags().HasTagExact(CXVIGameplayTags::Ability_Parry))
		{
			if (UCXGA_Dodge* Dodge = Cast<UCXGA_Dodge>(Spec->GetPrimaryInstance()))
			{
				NextDefenseAllowedTime = 0.0;
				Dodge->FinishSuccessfulReaction();
			}
		}
	}

	BroadcastDamageResult(InInstigator, ECXDamageResult::Dodged, 0.f);
	StartDodgeMontage();
}

void ACXCombatCharacterBase::BroadcastDamageResult(AActor* InInstigator, ECXDamageResult InResult, float InAmount) const
{
	FCXDamageResultMessage Message;
	Message.Target = const_cast<ACXCombatCharacterBase*>(this);
	Message.Instigator = InInstigator;
	Message.Result = InResult;
	Message.Amount = InAmount;

	UCXGameplayMessageSubsystem::Get(this).BroadcastMessage(CXVIGameplayTags::Message_Combat_DamageResult, Message);
}

float ACXCombatCharacterBase::GetCombatSpeed() const
{
	return CharacterAttributeSet ? CharacterAttributeSet->GetCombatSpeed() : 0.f;
}

void ACXCombatCharacterBase::CombatMessageRequest(const FCXCombatMessageData& MessageRequest)
{
	SavedMessage = MessageRequest;

	ProcessMessage(SavedMessage.Phase);
}

void ACXCombatCharacterBase::SubmitAction(const FCXActionSelection& Selection)
{
	if (!bIsTurn || SavedMessage.Phase != ECXCombatPhase::SelectActionPhase)
	{
		UE_LOG(LogTemp, Display, TEXT("폰에서 전달받은 액션"));
		return;
	}

	PendingAction = Selection;

	SavedMessage.Character = this;
	SavedMessage.MessageType = PendingAction.TargetTeam == ECXTargetTeam::Player 
		? ECXCombatMessageType::TeamTargetRequest : ECXCombatMessageType::EnemyTargetsRequest;

	CombatMessageResponse(SavedMessage);
}

void ACXCombatCharacterBase::SubmitTarget(APawn* Target)
{
	if (!bIsTurn ||	SavedMessage.Phase != ECXCombatPhase::SelectTargetPhase)
	{
		return;
	}

	ACXCombatCharacterBase* CombatTarget = Cast<ACXCombatCharacterBase>(Target);

	if (!IsValid(CombatTarget) || !CombatTarget->GetIsCharacterAlive())
	{
		return;
	}

	const bool bCandidate = SavedMessage.Targets.ContainsByPredicate(
		[Target](const TWeakObjectPtr<APawn>& Candidate)
		{
			return Candidate.Get() == Target;
		});

	if (!bCandidate)
	{
		return;
	}

	CurrentTargetPawn = Target;

	bDoingAction = true;
	UE_LOG(LogTemp, Display, TEXT("행동 시작"));

	FCXCombatMessageData Response = SavedMessage;
	Response.Character = this;
	Response.MessageType = ECXCombatMessageType::None;

	CombatMessageResponse(Response);
}

void ACXCombatCharacterBase::CancelSelection()
{
	if (!bIsTurn || SavedMessage.Phase != ECXCombatPhase::SelectTargetPhase)
	{
		return;
	}

	PendingAction.Clear();
	CurrentTargetPawn.Reset();
	SavedMessage.Targets.Reset();

	FCXCombatMessageData Response = SavedMessage;
	Response.Character = this;
	Response.MessageType = ECXCombatMessageType::SelectionCancelled;

	CombatMessageResponse(Response);
}

bool ACXCombatCharacterBase::ExecuteCombatAbility()
{
	return BasicAttackAbility();
}

void ACXCombatCharacterBase::CompleteCombatBehavior()
{
	

	bWaitingForReturn = false;

	if (SavedMessage.Phase == ECXCombatPhase::ReturnPhase)
	{
		FCXCombatMessageData Response;
		Response.Character = this;
		Response.Phase = ECXCombatPhase::ReturnPhase;
		Response.MessageType = ECXCombatMessageType::ReturnCompleted;

		CombatMessageResponse(Response);
	}
}

void ACXCombatCharacterBase::ReturnToOriginTransform()
{
	ACXCombatAIController* AIController = Cast<ACXCombatAIController>(GetController());

	if (!AIController || !AIController->EndCombatAction())
	{
		return;
	}

	GetCharacterMovement()->StopMovementImmediately();

	const FVector CurrentLocation = GetActorLocation();
	const FVector OriginLocation = OriginTransform.GetLocation();

	const float Error2D = FVector::Dist2D(CurrentLocation, OriginLocation);

	UE_LOG(LogTemp, Display, TEXT("%s 복귀 위치 오차 : %.2f cm"), *GetName(), Error2D);

	if (Error2D <= 15.f)
	{
		FVector CorrectedLocation = CurrentLocation;
		CorrectedLocation.X = OriginLocation.X;
		CorrectedLocation.Y = OriginLocation.Y;

		SetActorLocation(CorrectedLocation);
	}

	// 위치는 Move To로 돌아오고 방향만 복원.
	SetActorRotation(OriginTransform.GetRotation().Rotator());

	bIsTurnPresentationActive = false;

	bIsReturningToOrigin = false;

	FCXCombatMessageData Response;
	Response.Character = this;
	Response.MessageType = ECXCombatMessageType::ReturnedToOrigin;
	CombatMessageResponse(Response);
}

void ACXCombatCharacterBase::CancelCombatAbility()
{
	if (ASC && ExecutingCombatAbility.IsValid())
	{
		const FGameplayAbilitySpecHandle Handle = ExecutingCombatAbility;

		ASC->CancelAbilityHandle(Handle);
	}
}


bool ACXCombatCharacterBase::BasicAttackAbility()
{
	if (!IsValid(ASC) || !bIsCharacterAlive)
	{
		return false;
	}

	FScopedAbilityListLock AbilityListLock(*ASC);

	FGameplayTagContainer AttackTags;
	AttackTags.AddTag(CXVIGameplayTags::Ability_Basic_Attack);

	TArray<FGameplayAbilitySpec*> MatchingAbilities;

	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
		AttackTags,
		MatchingAbilities,
		false
	);

	FGameplayAbilitySpec* Picked = MatchingAbilities[FMath::RandRange(0, MatchingAbilities.Num() - 1)];

	if (!Picked || !Picked->Ability)
	{
		return false;
	}

	if (Picked->IsActive())
	{
		return false;
	}

	
	Picked->InputPressed = true;

	return TryStartCombatAbility(Picked->Handle);

}

bool ACXCombatCharacterBase::DodgeAbility()
{
	if (!IsValid(ASC) || !bIsCharacterAlive || bIsReturningToOrigin)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const double Now = World->GetTimeSeconds();
	if (Now < NextDefenseAllowedTime)
	{
		return false;
	}

	FScopedAbilityListLock AbilityListLock(*ASC);

	FGameplayTagContainer DodgeTag;
	DodgeTag.AddTag(CXVIGameplayTags::Ability_Dodge);

	TArray<FGameplayAbilitySpec*> MatchingAbilities;

	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
		DodgeTag,
		MatchingAbilities,
		false
	);

	for (FGameplayAbilitySpec* Spec : MatchingAbilities)
	{
		if (!Spec || !Spec->Ability || Spec->IsActive() ||
			Spec->Ability->GetAssetTags().HasTagExact(CXVIGameplayTags::Ability_Parry))
		{
			continue;
		}

		// 캐릭터당 하나의 행동만 실행.
		if (ExecutingCombatAbility.IsValid())
		{
			return false;
		}

		const FGameplayAbilitySpecHandle Handle = Spec->Handle;

		ExecutingCombatAbility = Handle;
		CurrentActivatingHandles.AddUnique(Handle);
		bDoingAction = true;

		const double PreviousDefenseAllowedTime = NextDefenseAllowedTime;
		constexpr double CooldownSeconds = 1.0;
		NextDefenseAllowedTime = Now + CooldownSeconds;

		if (!ASC->TryActivateAbility(Handle))
		{
			NextDefenseAllowedTime = PreviousDefenseAllowedTime;
			CurrentActivatingHandles.Remove(Handle);
			ExecutingCombatAbility = FGameplayAbilitySpecHandle();
			bDoingAction = false;

			return false;
		}
		return true;
	}

	// 사용할 회피 어빌리티가 없음.
	return false;
}

bool ACXCombatCharacterBase::ParryAbility()
{
	if (!IsValid(ASC) || !bIsCharacterAlive || bIsReturningToOrigin)
	{
		return false;
	}

	if (!CanParryIncomingAttack())
	{
		UE_LOG(LogTemp, Display,
			TEXT("[패링] %s: 이번 공격은 패링할 수 없습니다."),
			*GetName());

		return false;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const double Now = World->GetTimeSeconds();
	if (Now < NextDefenseAllowedTime)
	{
		UE_LOG(LogTemp, Display, TEXT("[패리] %s / 남은 쿨타임 %.2f초"),
			*GetName(), NextDefenseAllowedTime - Now);
		return false;
	}

	FScopedAbilityListLock AbilityListLock(*ASC);

	FGameplayTagContainer DodgeTag;
	DodgeTag.AddTag(CXVIGameplayTags::Ability_Parry);

	TArray<FGameplayAbilitySpec*> MatchingAbilities;

	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
		DodgeTag,
		MatchingAbilities,
		false
	);

	for (FGameplayAbilitySpec* Spec : MatchingAbilities)
	{
		if (!Spec || !Spec->Ability)
		{
			continue;
		}

		if (Spec->IsActive())
		{
			return false;
		}

		// 캐릭터당 하나의 행동만 실행.
		if (ExecutingCombatAbility.IsValid())
		{
			return false;
		}

		const FGameplayAbilitySpecHandle Handle = Spec->Handle;

		ExecutingCombatAbility = Handle;
		CurrentActivatingHandles.AddUnique(Handle);
		bDoingAction = true;

		const double PreviousParryAllowedTime = NextDefenseAllowedTime;
		constexpr double CooldownSeconds = 1.0;
		NextDefenseAllowedTime = Now + CooldownSeconds;

		if (!ASC->TryActivateAbility(Handle))
		{
			NextDefenseAllowedTime = PreviousParryAllowedTime;
			CurrentActivatingHandles.Remove(Handle);
			ExecutingCombatAbility = FGameplayAbilitySpecHandle();
			bDoingAction = false;
			return false;
		}

		UE_LOG(LogTemp, Display, TEXT("[패리] %s / 쿨타임 %.1f초 시작"),
			*GetName(), CooldownSeconds);
		return true;
	}

	// 사용할 회피 어빌리티가 없음.
	return false;
}

bool ACXCombatCharacterBase::CounterAbility()
{
	if (!IsValid(ASC) || !bIsCharacterAlive)
	{
		return false;
	}

	FScopedAbilityListLock AbilityListLock(*ASC);

	FGameplayTagContainer DodgeTag;
	DodgeTag.AddTag(CXVIGameplayTags::Ability_Counter_Attack);

	TArray<FGameplayAbilitySpec*> MatchingAbilities;

	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
		DodgeTag,
		MatchingAbilities,
		false
	);

	for (FGameplayAbilitySpec* Spec : MatchingAbilities)
	{
		if (!Spec || !Spec->Ability)
		{
			continue;
		}

		if (Spec->IsActive())
		{
			return false;
		}

		// 캐릭터당 하나의 행동만 실행.
		if (ExecutingCombatAbility.IsValid())
		{
			return false;
		}

		const FGameplayAbilitySpecHandle Handle = Spec->Handle;

		ExecutingCombatAbility = Handle;
		CurrentActivatingHandles.AddUnique(Handle);
		bDoingAction = true;


		if (const UCXCombatManagerSubsystem* CombatManager =
			UCXCombatManagerSubsystem::TryGet(this))
		{
			CombatManager->RequestCameraPreset(TEXT("Counter.Action"), this);
		}


		if (!ASC->TryActivateAbility(Handle))
		{
			CurrentActivatingHandles.Remove(Handle);
			ExecutingCombatAbility = FGameplayAbilitySpecHandle();
			bDoingAction = false;

			return false;
		}

		return true;
	}

	// 사용할 회피 어빌리티가 없음
	
	return false;
}

void ACXCombatCharacterBase::TeleportToAttackLocation()
{
	APawn* Target = CurrentTargetPawn.Get();

	if (!IsValid(Target))
	{
		return;
	}

	FVector AttackPosition = Target->GetActorLocation() + Target->GetActorForwardVector() * 50.f;

	SetActorLocation(AttackPosition);
}

void ACXCombatCharacterBase::ReturnToOriginLocation()
{
	SetActorTransform(OriginTransform);


}

bool ACXCombatCharacterBase::TryStartCombatAbility(FGameplayAbilitySpecHandle Handle)
{
	if (!ASC || !Handle.IsValid() || ExecutingCombatAbility.IsValid())
	{
		return false;
	}

	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);

	if (!Spec || Spec->IsActive())
	{
		return false;
	}

	ACXCombatCharacterBase* Target = Cast<ACXCombatCharacterBase>(CurrentTargetPawn.Get());

	if (!IsValid(Target) || !Target->GetIsCharacterAlive())
	{
		return false;
	}

	const auto* AttackAbility = Cast<UCXGameplayAbilityCombatBase>(Spec->Ability);
	const ECXAttackTargetMode TargetMode = AttackAbility
		? AttackAbility->GetAttackTargetMode()
		: ECXAttackTargetMode::Single;

	if (TargetMode == ECXAttackTargetMode::Single)
	{
		FVector Direction = Target->GetActorLocation() - GetActorLocation();
		Direction.Z = 0.f;

		if (!Direction.IsNearlyZero())
		{
			SetActorRotation(Direction.Rotation());
		}
	}

	// 새 행동에서는 다시 한번 마나 획득 가능
	bBasicAttackManaGranted = false;

	ExecutingCombatAbility = Handle;
	CurrentActivatingHandles.AddUnique(Handle);

	if (!ASC->TryActivateAbility(Handle))
	{
		CurrentActivatingHandles.Remove(Handle);
		ExecutingCombatAbility = FGameplayAbilitySpecHandle();

		return false;
	}
	return true;
}

bool ACXCombatCharacterBase::ShouldApproachTarget() const
{
	if (const FGameplayAbilitySpec* Spec = GetSelectedAttackSpec())
	{
		if (const auto* Skill = Cast<UCXGameplayAbilityCombatBase>(Spec->Ability))
		{
			return Skill->ShouldApproachTarget();
		}
	}

	return true;
}

FOnTurnPhaseFinished& ACXCombatCharacterBase::GetPhaseFinishedDelegate()
{
	return PhaseFinishedEvent;
}

void ACXCombatCharacterBase::ProcessMessage(const ECXCombatPhase& CurPhase)
{
	// 현재 페이즈에 맞게 처리
	switch (CurPhase)
	{
	case ECXCombatPhase::BeginPhase:BeginTurnPhase();	break;
	case ECXCombatPhase::BeforeSelectPhase:BeforeSelectPhase(); break;
	case ECXCombatPhase::SelectActionPhase:SelectActionPhase(); break;
	case ECXCombatPhase::SelectTargetPhase:SelectTargetPhase(); break;
	case ECXCombatPhase::ActionPhase:
	{
		if (!ActionPhase())
		{
			bDoingAction = false;
			FinishAction();
		}
		break;
	}
	case ECXCombatPhase::ReturnPhase:ReturnPhase(); break;
	case ECXCombatPhase::EndPhase:EndTurnPhase(); break;
	}
}

void ACXCombatCharacterBase::BeginTurnPhase()
{
	bIsTurn = true;
	bIsTurnPresentationActive = true;

	UE_LOG(LogTemp, Display, TEXT("%s: 턴시작"), *GetName());

	// 턴시작 처리
	SavedMessage.Character = this;

	CombatMessageResponse(SavedMessage);
}

void ACXCombatCharacterBase::BeforeSelectPhase()
{
	// 전투이외 데미지 입는 처리

	SavedMessage.Character = this;
	CombatMessageResponse(SavedMessage);
}

void ACXCombatCharacterBase::SelectActionPhase()
{
	// 선택지를 선택하는 곳
	// 적을 공격, 아군을 힐, 건너뛰기, 도망, 자유공격
	// 단일공격시 매니저에게 적 배열을 받아와야함

	SavedMessage.Character = this;
	SavedMessage.MessageType = ECXCombatMessageType::EnemyTargetsRequest;
	CombatMessageResponse(SavedMessage);
	// 아군을 선택하는 스킬도 아군 배열을 받아와야함

	// 건너뛰기는 매니저에서 턴종료로 이동

	// 자유공격은 행동후 다시 선택지페이즈로 돌아와야함
}

void ACXCombatCharacterBase::SelectTargetPhase()
{
	
}

bool ACXCombatCharacterBase::ActionPhase()
{

	ACXCombatCharacterBase* Target = Cast<ACXCombatCharacterBase>(CurrentTargetPawn.Get());

	if (!IsValid(Target) || !Target->GetIsCharacterAlive())
	{
		return false;
	}

	if (!GetController())
	{
		SpawnDefaultController();
	}

	ACXCombatAIController* AIController = Cast<ACXCombatAIController>(GetController());
	
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s : CombatAicontroller가 없음"), *GetName());

		return false;
	}

	const float Distance = GetCapsuleComponent()->GetScaledCapsuleRadius() + Target->GetCapsuleComponent()->GetScaledCapsuleRadius() + 25.f;

	const FTransform TargetAnchor = PendingAction.Type == ECXActionType::CounterAttack
		? Target->GetActorTransform()
		: Target->GetOriginTransform();

	FVector AttackLocation =
		TargetAnchor.GetLocation() + TargetAnchor.GetUnitAxis(EAxis::X) * Distance;
	AttackLocation.Z = Target->GetActorLocation().Z;

	const bool bShouldApproach = ShouldApproachTarget();
	const FGameplayAbilitySpec* Spec = GetSelectedAttackSpec();
	const auto* AttackAbility = Spec ? Cast<UCXGameplayAbilityCombatBase>(Spec->Ability) : nullptr;

	if (bShouldApproach && AttackAbility &&
		AttackAbility->GetAttackTargetMode() == ECXAttackTargetMode::AllOpponents)
	{
		const auto* CombatManager = UCXCombatManagerSubsystem::TryGet(this);
		const auto* Arena = CombatManager ? CombatManager->GetCombatArena() : nullptr;
		if (IsValid(Arena))
		{
			AttackLocation = Arena->GetAreaAttackLocation();
			UE_LOG(LogTemp, Display, TEXT("[범위 이동] %s -> %s"),
				*GetName(), *AttackLocation.ToString());
		}
	}

	// 접근 이동부터 공격 종료 까지 행동 중으로 취급
	bDoingAction = true;

	bWaitingForReturn = true;



	if (!AIController->RequestCombatAction(Target, AttackLocation, OriginTransform.GetLocation(), bShouldApproach))
	{

		bDoingAction = false;
		bWaitingForReturn = false;
		return false;
	}

	// 접근 이동 전에 알림. 반격도 이 경로로 들어옴 (ActionType = CounterAttack)
	BroadcastActionStarted();

	return true;
}

void ACXCombatCharacterBase::BroadcastActionStarted() const
{
	FCXCombatActionStartedMessage Message;
	Message.Character = const_cast<ACXCombatCharacterBase*>(this);
	Message.ActionType = GetSelectedActionType();
	Message.ActionName = GetSelectedActionName();

	UCXGameplayMessageSubsystem::Get(this).BroadcastMessage(CXVIGameplayTags::Message_Combat_ActionStarted, Message);
}

void ACXCombatCharacterBase::ReturnPhase()
{
	// 적의 턴일때 패링이나 회피 리액션을 취할 수 있다
	// 아직 패링회피 구현안되어서 이페이즈가 오면 바로 스킵
	/*SavedMessage.Character = this;
	SavedMessage.Phase = ECXCombatPhase::ReturnPhase;
	SavedMessage.MessageType = ECXCombatMessageType::None;*/

	//CombatMessageResponse(SavedMessage);
	ACXCombatAIController* AIController = Cast<ACXCombatAIController>(GetController());

	if (!AIController || !AIController->RequestCombatReturn())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 복귀 허가 실패"), *GetName());

		return;
	}

	bIsReturningToOrigin = true;
	UE_LOG(LogTemp, Display, TEXT("%s: 복귀 페이즈 진입, 복귀 허가"), *GetName());

}

void ACXCombatCharacterBase::EndTurnPhase()
{
	UE_LOG(LogTemp, Display, TEXT("%s: 턴종료"), *GetName());

	// 턴 끝났음
	bIsTurn = false;

	// 턴 끝났음을 보냄
	SavedMessage.Character = this;
	SavedMessage.MessageType = ECXCombatMessageType::TurnCompleted;
	CombatMessageResponse(SavedMessage);
}

void ACXCombatCharacterBase::TryGainBasicAttackMana()
{
	if (!HasAuthority() || !IsValid(ASC) ||
		!bIsCharacterAlive || bBasicAttackManaGranted || !ExecutingCombatAbility.IsValid() || !BasicAttackManaGainEffect)
	{
		return;
	}

	const FGameplayAbilitySpec* AbilitySpec = ASC->FindAbilitySpecFromHandle(ExecutingCombatAbility);

	if (!AbilitySpec || !AbilitySpec->Ability)
	{
		return;
	}

	// 현재 실행중인 행동이 기본공격인지 확인.
	const bool bIsBasicAttack = AbilitySpec->Ability->GetAssetTags().HasTagExact(CXVIGameplayTags::Ability_Basic_Attack);

	if (!bIsBasicAttack)
	{
		return;
	}

	FGameplayEffectSpecHandle EffectSpec = ASC->MakeOutgoingSpec(BasicAttackManaGainEffect, 1.f, ASC->MakeEffectContext());

	if (!EffectSpec.IsValid())
	{
		return;
	}

	// GE적용으로 추가 콜백이 발생해도 중복 획득하지 않도록 적용전에 기록
	bBasicAttackManaGranted = true;

	ASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
}

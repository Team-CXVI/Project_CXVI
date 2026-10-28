// Fill out your copyright notice in the Description page of Project Settings.


#include "CXCombatCharacterBase.h"

#include "CXVI/Tags/CXGameplayTags.h"

#include "CXVI/AbilitySystem/Attributes/Combat/CXCombatAttributeSet.h"

#include "AI/Combat/CXCombatAIController.h"

#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"

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

	// Movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;

	// Mesh
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -100.0f), FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	GetMesh()->SetCollisionProfileName(TEXT("NoCollision"));

	// Widget
	HpBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("HpBar"));
	HpBar->SetupAttachment(GetMesh());
	HpBar->SetRelativeLocation(FVector(0.f, 0.f, 180.f));
	HpBar->SetWidgetSpace(EWidgetSpace::Screen);
	HpBar->SetDrawSize(FVector2D(150.f, 20.f));
	HpBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// ASC
	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));

	// AttributeSet
	CharacterAttributeSet = CreateDefaultSubobject<UCXCombatAttributeSet>(TEXT("CharacterAttributeSet"));
	ASC->AddAttributeSetSubobject<UCXCombatAttributeSet>(CharacterAttributeSet);

	// 캐릭터 기본변수 초기화
	bIsCharacterAlive = true;
	bIsTurn = false;

	AIControllerClass = ACXCombatAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
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

	CXPostInitializeComponents();

	ASC->GetSet<UCXCombatAttributeSet>()->OnOutOfHealth.AddDynamic(this, &ACXCombatCharacterBase::OnOutOfHealthCpp);

	CharacterAttributeSet->OnDamageApplied.AddUniqueDynamic(this, &ACXCombatCharacterBase::HandleDamageApplied);
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

	bDoingAction = false;
	UE_LOG(LogTemp, Display, TEXT("행동 종료"));

	// BT에 공격 종료 알림
	if(bIsTurn)
	{ 
		OnCombatAbilityFinished.Broadcast(bWasCancelled);
	}

	// 턴매니저에게도 공격 종료 알림
	FinishAction();
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
	HpBar->SetVisibility(false);

	bIsCharacterAlive = false;
}

void ACXCombatCharacterBase::HandleDamageApplied(AActor* InInstigator, float AppliedDamage)
{
	if (AppliedDamage <= 0.f)
	{
		return;
	}

	ACXCombatCharacterBase* Attacker = Cast<ACXCombatCharacterBase>(InInstigator);

	// this는 본인이 치고 본인이 맞은 경우는 제거
	if (!IsValid(Attacker) || Attacker == this)
	{
		return;
	}

	Attacker->TryGainBasicAttackMana();

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

	// 다음 턴이 시작될수있으므로 마지막에 호출.
	if (bIsTurn && SavedMessage.Phase == ECXCombatPhase::ReturnPhase)
	{
		FCXCombatMessageData Response = SavedMessage;
		Response.Character = this;
		Response.MessageType = ECXCombatMessageType::None;

		CombatMessageResponse(Response);
	}
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

		Spec->InputPressed = true;

		return TryStartCombatAbility(Spec->Handle);
	}

	// 사용할 기본 공격 어빌리티가 없음.
	return false;
}

bool ACXCombatCharacterBase::DodgeAbility()
{
	if (!IsValid(ASC) || !bIsCharacterAlive)
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

		if (!ASC->TryActivateAbility(Handle))
		{
			CurrentActivatingHandles.Remove(Handle);
			ExecutingCombatAbility = FGameplayAbilitySpecHandle();
			bDoingAction = false;

			return false;
		}
	}

	// 사용할 회피 어빌리티가 없음.
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

	FVector Direction = Target->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.f;

	if (!Direction.IsNearlyZero())
	{
		SetActorRotation(Direction.Rotation());
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
	case ECXCombatPhase::ActionPhase:ActionPhase(); break;
	case ECXCombatPhase::ReturnPhase:ReturnPhase(); break;
	case ECXCombatPhase::EndPhase:EndTurnPhase(); break;
	}
}

void ACXCombatCharacterBase::BeginTurnPhase()
{
	bIsTurn = true;

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

void ACXCombatCharacterBase::ActionPhase()
{
	ACXCombatCharacterBase* Target = Cast<ACXCombatCharacterBase>(CurrentTargetPawn.Get());

	if (!IsValid(Target) || !Target->GetIsCharacterAlive())
	{
		FinishAction();
		return;
	}

	if (!GetController())
	{
		SpawnDefaultController();
	}

	ACXCombatAIController* AIController = Cast<ACXCombatAIController>(GetController());
	
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s : CombatAicontroller가 없음"), *GetName());

		FinishAction();
		return;
	}

	const float Distance = GetCapsuleComponent()->GetScaledCapsuleRadius() + Target->GetCapsuleComponent()->GetScaledCapsuleRadius() + 25.f;

	const FVector AttackLocation = Target->GetActorLocation() + Target->GetActorForwardVector() * Distance;

	// 접근 이동부터 공격 종료 까지 행동 중으로 취급
	bDoingAction = true;

	if (!AIController->RequestCombatAction(Target, AttackLocation, OriginTransform.GetLocation(), ShouldApproachTarget()))
	{

		bDoingAction = false;
		FinishAction();
	}
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

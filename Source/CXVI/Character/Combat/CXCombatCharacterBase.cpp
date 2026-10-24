// Fill out your copyright notice in the Description page of Project Settings.


#include "CXCombatCharacterBase.h"

#include "CXVI/Tags/CXGameplayTags.h"

#include "CXVI/AbilitySystem/Attributes/Combat/CXCombatAttributeSet.h"

#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"

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
	GetCharacterMovement()->MaxWalkSpeed = 300.f;
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
	if (!CurrentActivatingHandles.Contains(Handle))
	{
		return;
	}

	CurrentActivatingHandles.Remove(Handle);

	if (!CurrentActivatingHandles.IsEmpty())
	{
		return;
	}

	ReturnToOriginLocation();

	// 이 예시는 중단된 행동도 종료 처리하여 턴을 소비한다.
	FinishAction();
}

void ACXCombatCharacterBase::FinishAction()
{
	if (!bIsTurn || SavedMessage.Phase != ECXCombatPhase::ActionPhase)
	{
		return;
	}

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

		Spec->InputPressed = true;

		// 이미 공격 중이면 기존 콤보 입력 처리.
		if (Spec->IsActive())
		{
			ASC->AbilitySpecInputPressed(*Spec);
			return true;
		}

		ACXCombatCharacterBase* Target = Cast<ACXCombatCharacterBase>(CurrentTargetPawn.Get());

		if (!IsValid(Target) || !Target->GetIsCharacterAlive())
		{
			return false;
		}

		const FGameplayAbilitySpecHandle Handle = Spec->Handle;

		TeleportToAttackLocation();

		// 활성화 직후 종료 콜백이 올 수 있으므로 먼저 등록.
		CurrentActivatingHandles.AddUnique(Handle);

		if (!ASC->TryActivateAbility(Handle))
		{
			CurrentActivatingHandles.Remove(Handle);
			ReturnToOriginLocation();

			return false;
		}

		// 정상 종료 처리는 NotifyBattleAbilityEnded()가 담당.
		return true;
	}

	// 사용할 기본 공격 어빌리티가 없음.
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
	case ECXCombatPhase::ReactionPhase:ReactionPhase(); break;
	case ECXCombatPhase::EndPhase:EndTurnPhase(); break;
	}
}

void ACXCombatCharacterBase::BeginTurnPhase()
{
	bIsTurn = true;

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
	// 이전 페이즈에서 누군가를 선택해야하는 선택지를 골랐을경우

	// 적이나 아군중에 선택을 해야함
	for (auto& WeakTarget : SavedMessage.Targets)
	{
		if (ACXCombatCharacterBase* Target = Cast<ACXCombatCharacterBase>(WeakTarget.Get()))
		{
			if (Target->GetIsCharacterAlive())
			{
				CurrentTargetPawn = Target;
			}
		}
	}


	// 선택을 하지않고 취소를 할경우 이전 페이즈로 돌아간다.

	SavedMessage.Character = this;
	SavedMessage.MessageType = ECXCombatMessageType::None;
	CombatMessageResponse(SavedMessage);
}

void ACXCombatCharacterBase::ActionPhase()
{
	// 선택한 행동을 실행한다

	// 예를들면 스킬애니메이션 발동

	// BasicAttack()
}

void ACXCombatCharacterBase::ReactionPhase()
{
	// 적의 턴일때 패링이나 회피 리액션을 취할 수 있다
	// 아직 패링회피 구현안되어서 이페이즈가 오면 바로 스킵
	SavedMessage.Character = this;
	SavedMessage.Phase = ECXCombatPhase::ReactionPhase;
	SavedMessage.MessageType = ECXCombatMessageType::None;

	CombatMessageResponse(SavedMessage);
}

void ACXCombatCharacterBase::EndTurnPhase()
{
	// 턴 끝났음
	bIsTurn = false;

	// 턴 끝났음을 보냄
	SavedMessage.Character = this;
	SavedMessage.MessageType = ECXCombatMessageType::TurnCompleted;
	CombatMessageResponse(SavedMessage);
}

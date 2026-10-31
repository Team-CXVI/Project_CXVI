// Fill out your copyright notice in the Description page of Project Settings.

#include "CXFieldPlayerCharacter.h"
#include "InputActionValue.h"
#include "CXVI/Tags/CXFieldTags.h"
#include "CXVI/Actor/Field/Interactions/CXExpeditionFlag.h"

#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "AbilitySystemComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "GameplayEffect.h"
#include "InputAction.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "AnimationWarpingLibrary.h"
#include "MotionWarpingComponent.h"
#include "CableComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldPlayerCharacter)

ACXFieldPlayerCharacter::ACXFieldPlayerCharacter()
{
	GrapplingCable = CreateDefaultSubobject<UCableComponent>(TEXT("GrapplingCable"));
	GrapplingCable->SetupAttachment(GetMesh());

	GrapplingCable->bAttachStart = true;
	GrapplingCable->bAttachEnd = true;

	GrapplingCable->CableWidth = 2.0f;
	GrapplingCable->CableGravityScale = 0.0f;

	// 처음에는 곧은 줄로 구성.
	GrapplingCable->NumSegments = 1;
	GrapplingCable->bEnableCollision = false;

	// 숨겨진 동안 케이블 업데이트 중단.
	GrapplingCable->bSkipCableUpdateWhenNotVisible = true;
	GrapplingCable->SetVisibility(false);
	GrapplingCable->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACXFieldPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(ASC))
	{
		// 기존 활성 어빌리티도 포함하고, 이후 시작/종료는 이벤트로 추적합니다.
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			ActiveInteractionUIAbilityCount += Spec.ActiveCount;
		}
		InteractionUIAbilityActivatedHandle = ASC->AbilityActivatedCallbacks.AddUObject(
			this, &ThisClass::HandleInteractionUIAbilityActivated);
		InteractionUIAbilityEndedHandle = ASC->AbilityEndedCallbacks.AddUObject(
			this, &ThisClass::HandleInteractionUIAbilityEnded);
		ASC->SetLooseGameplayTagCount(CXVIFieldTag::State_BlockInteractionUI,
			ActiveInteractionUIAbilityCount > 0 ? 1 : 0);
		RefreshAirborneTag();
	}



	if (SprintAbility)
	{
		FGameplayAbilitySpec SprintSpec(
			SprintAbility,
			1,
			INDEX_NONE,
			this
		);

		SprintAbilityHandle = ASC->GiveAbility(SprintSpec);
	}

	if (IsValid(ASC) && GrapplingAbility)
	{
		FGameplayAbilitySpec GrapplingHookSpec(
			GrapplingAbility,
			1,
			INDEX_NONE,
			this);
		GrapplingHookHandle = ASC->GiveAbility(GrapplingHookSpec);
	}

	if (IsValid(ASC) && PreemptiveAttackAbility)
	{
		FGameplayAbilitySpec PreemptiveAttackSpec(
			PreemptiveAttackAbility,
			1,
			INDEX_NONE,
			this);
		PreemptiveAttackHandle = ASC->GiveAbility(PreemptiveAttackSpec);
	}

	if (IsValid(ASC) && AttackAbility)
	{
		FGameplayAbilitySpec AttackSpec(
			AttackAbility,
			1,
			INDEX_NONE,
			this
		);
		AttackAbilityHandle = ASC->GiveAbility(AttackSpec);
	}

	if (IsValid(ASC) && RopeTraversalAbility)
	{
		FGameplayAbilitySpec RopeTraversalAbilitySpec(
			RopeTraversalAbility,
			1,
			INDEX_NONE,
			this
		);
		RopeTraversalAbilityHandle = ASC->GiveAbility(RopeTraversalAbilitySpec);
	}
}


void ACXFieldPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(ASC))
	{
		ASC->AbilityActivatedCallbacks.Remove(InteractionUIAbilityActivatedHandle);
		ASC->AbilityEndedCallbacks.Remove(InteractionUIAbilityEndedHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ACXFieldPlayerCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	RefreshAirborneTag();
}

void ACXFieldPlayerCharacter::RefreshAirborneTag()
{
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (IsValid(ASC) && IsValid(Movement))
	{
		ASC->SetLooseGameplayTagCount(CXVIFieldTag::State_Airborne,
			Movement->IsMovingOnGround() ? 0 : 1);
	}
}

void ACXFieldPlayerCharacter::HandleInteractionUIAbilityActivated(UGameplayAbility* Ability)
{
	++ActiveInteractionUIAbilityCount;
	if (IsValid(ASC))
	{
		ASC->SetLooseGameplayTagCount(CXVIFieldTag::State_BlockInteractionUI, 1);
	}
}

void ACXFieldPlayerCharacter::HandleInteractionUIAbilityEnded(UGameplayAbility* Ability)
{
	ActiveInteractionUIAbilityCount = FMath::Max(ActiveInteractionUIAbilityCount - 1, 0);
	if (IsValid(ASC))
	{
		ASC->SetLooseGameplayTagCount(CXVIFieldTag::State_BlockInteractionUI,
			ActiveInteractionUIAbilityCount > 0 ? 1 : 0);
	}
}

void ACXFieldPlayerCharacter::SetGrapplingTarget(AActor* TargetActor, AActor* LandingActor)
{
	// 그래플링 대상과 착지 위치를 저장한다.
	CurrentGrapplingTarget = TargetActor;
	CurrentLandingSurface = IsValid(TargetActor) ? LandingActor : nullptr;
}

void ACXFieldPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//if (InputMappingContext)
	//{
	//	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	//	{
	//		
	//		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	//		{
	//			Subsystem->AddMappingContext(InputMappingContext, 0);
	//		}
	//	}
	//}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ThisClass::StopSprint);
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Canceled, this, &ThisClass::StopSprint
			);
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
		}

		if (InteractiveAction)
		{
			EnhancedInputComponent->BindAction(InteractiveAction, ETriggerEvent::Started, this, &ThisClass::Interactive);
		}

		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::StartJumpOrParkour);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::ReleaseJumpOrParkour);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ThisClass::ReleaseJumpOrParkour);
		}

		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction,ETriggerEvent::Started,this,&ThisClass::StartSprint);

			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ThisClass::StopSprint);

			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ThisClass::StopSprint);
		}

		if (AttackAction)
		{
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ThisClass::AttackFunc);
		}
	}

}

void ACXFieldPlayerCharacter::SetRopeTraversalTarget(ACXRopeTraversalActor* Rope, ECXRopeTraversalDirection Direction)
{
	if (!IsValid(Rope) || Direction == ECXRopeTraversalDirection::None)
	{
		return;
	}
	
	CurrentRopeTraversalTarget = Rope;
	CurrentRopeTraversalDirection = Direction;

	if (IsValid(ASC))
	{
		ASC->SetLooseGameplayTagCount(CXVIFieldTag::State_CanUseRope, 1);
	}

	UE_LOG(LogTemp, Log, TEXT("[RopeTarget] 등록: %s / %s"), *GetNameSafe(Rope), Direction == ECXRopeTraversalDirection::Up ? TEXT("상승") : TEXT("하강"));
	UE_LOG(LogTemp, Log, TEXT("[RopeTarget] 사용 가능: %d"),
		IsValid(ASC) &&
		ASC->HasMatchingGameplayTag(CXVIFieldTag::State_CanUseRope));
}

void ACXFieldPlayerCharacter::ClearRopeTraversalTarget(ACXRopeTraversalActor* Rope, ECXRopeTraversalDirection Direction)
{
	if (CurrentRopeTraversalTarget.Get() != Rope || CurrentRopeTraversalDirection != Direction)
	{
		return;
	}

	CurrentRopeTraversalTarget.Reset();
	CurrentRopeTraversalDirection = ECXRopeTraversalDirection::None;

	if (IsValid(ASC))
	{
		ASC->SetLooseGameplayTagCount(CXVIFieldTag::State_CanUseRope,0);
	}

	UE_LOG(LogTemp, Log, TEXT("[RopeTarget] 해제: %s"), *GetNameSafe(Rope));
	UE_LOG(LogTemp, Log, TEXT("[RopeTarget] 사용 가능: %d"),
		IsValid(ASC) &&
		ASC->HasMatchingGameplayTag(CXVIFieldTag::State_CanUseRope));
}

void ACXFieldPlayerCharacter::Move(const FInputActionValue& InValue)
{
	if (!Controller || (ASC && ASC->HasMatchingGameplayTag(CXVIFieldTag::State_MovingBlock)))
	{
		return;
	}

	const FVector2D MovementVector = InValue.Get<FVector2D>();

	if (MovementVector.IsNearlyZero())
	{
		return;
	}
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (IsValid(AnimInstance) && IsValid(FlagMontage) &&
		AnimInstance->Montage_IsActive(FlagMontage) &&
		AnimInstance->Montage_GetCurrentSection(FlagMontage) == FName(TEXT("SitIdle")))
	{
		AnimInstance->Montage_Stop(0.2f, FlagMontage);
	}

	const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	const FRotationMatrix RotationMatrix(YawRotation);
	AddMovementInput(RotationMatrix.GetUnitAxis(EAxis::X), MovementVector.Y);
	AddMovementInput(RotationMatrix.GetUnitAxis(EAxis::Y), MovementVector.X);
}

void ACXFieldPlayerCharacter::Look(const FInputActionValue& InValue)
{
	if (Controller)
	{
		const FVector2D LookAxis = InValue.Get<FVector2D>();
		AddControllerYawInput(LookAxis.X);
		AddControllerPitchInput(LookAxis.Y);
	}
}



void ACXFieldPlayerCharacter::StartUseRope()
{
	if (IsValid(ASC) && RopeTraversalAbilityHandle.IsValid())
	{
		ASC->TryActivateAbility(RopeTraversalAbilityHandle);
	}
}

void ACXFieldPlayerCharacter::ShowCable(AActor* Target)
{
	if (!IsValid(GrapplingCable) || !IsValid(Target) || !IsValid(Target->GetRootComponent()))
	{
		return;
	}

	// 시작점 캐릭터 손목 소켓.
	GrapplingCable->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, GrapplingHandSoket);

	// 끝점은 그래플링 타겟, 착지 액터와 구분.
	GrapplingCable->SetAttachEndToComponent(Target->GetRootComponent());

	GrapplingCable->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 끝점 컴포넌트 기준의 상대 위치.
	GrapplingCable->EndLocation = FVector::ZeroVector;

	GrapplingCable->CableLength = FVector::Distance(GrapplingCable->GetComponentLocation(), Target->GetRootComponent()->GetComponentLocation());

	GrapplingCable->SetVisibility(true);
}

void ACXFieldPlayerCharacter::HideCable()
{
	if (IsValid(GrapplingCable))
	{
		GrapplingCable->SetVisibility(false);
		GrapplingCable->SetAttachEndToComponent(nullptr);
	}
}

bool ACXFieldPlayerCharacter::CanStartFlag() const
{
	if (!IsValid(ASC))
	{
		return false;
	}

	// 다른 행동으로 이동이 제한됐거나, 이미 깃발 메뉴가 열린 상태.
	if (ASC->HasMatchingGameplayTag(CXVIFieldTag::State_MovingBlock) ||
		ASC->HasMatchingGameplayTag(CXVIFieldTag::State_InMenu))
	{
		return false;
	}

	return ASC->HasMatchingGameplayTag(CXVIFieldTag::State_CanUseExpeditionFlag);
}

void ACXFieldPlayerCharacter::RegisterNearByFlag(ACXExpeditionFlag* Flag)
{
	if (!IsValid(Flag))
	{
		return;
	}

	NearByActiveExpeditionFlag = Flag;
	
}

void ACXFieldPlayerCharacter::UnregisterNearByFlag(ACXExpeditionFlag* Flag)
{
	// 현재 저장된 깃발에서 벗어났을 때만 제거.
	if (NearByActiveExpeditionFlag.Get() != Flag)
	{
		return;
	}
	NearByActiveExpeditionFlag.Reset();
}

void ACXFieldPlayerCharacter::OnSceneSuspended_Implementation()
{
	SetActorHiddenInGame(true);
	GetCharacterMovement()->DisableMovement();
}

void ACXFieldPlayerCharacter::OnSceneResumed_Implementation()
{
	SetActorHiddenInGame(false);
	GetCharacterMovement()->SetDefaultMovementMode();
}

void ACXFieldPlayerCharacter::Interactive()
{
	// GrapplingHook StartUseFlag

	if (!IsValid(ASC))
	{
		return;
	}

	if (ASC->HasMatchingGameplayTag(CXVIFieldTag::State_CanUseExpeditionFlag))
	{
		StartUseFlag();
	}
	else if (ASC->HasMatchingGameplayTag(CXVIFieldTag::State_CanGrapplingHook))
	{
		GrapplingHook();
	}
	else if (ASC->HasMatchingGameplayTag(CXVIFieldTag::State_CanUseRope))
	{
		StartUseRope();
	}
	else
	{
		return;
	}

}

void ACXFieldPlayerCharacter::AttackFunc()
{
	if (!IsValid(ASC))
	{
		return;
	}
	
	if (ASC->HasMatchingGameplayTag(CXVIFieldTag::State_CanAssassination))
	{
		Assassination();
	}
	else
	{
		Attack();
		return;
	}

}

void ACXFieldPlayerCharacter::StartJumpOrParkour()
{
	if (bJumpOrParkourInputHeld)
	{
		return;
	}
	bJumpOrParkourInputHeld = true;

	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	const bool bParkourMontageActive = ActiveParkourMontage.IsValid()
		&& IsValid(AnimInstance)
		&& AnimInstance->Montage_IsActive(ActiveParkourMontage.Get());

	// 진행 중의 재입력을 버린다. 파쿠르 실패로 간주해 일반 점프를 호출하지 않는다.
	if (!IsValid(Movement)
		|| HasAnyRootMotion() || bParkourMontageActive
		|| (ASC && ASC->HasMatchingGameplayTag(CXVIFieldTag::State_MovingBlock)))
	{
		return;
	}

	ActiveParkourMontage.Reset();
	if (TryParkour())
	{
		if (IsValid(AnimInstance))
		{
			ActiveParkourMontage = AnimInstance->GetCurrentActiveMontage();
			AlignParkourMontageEntry(AnimInstance, ActiveParkourMontage.Get());
		}
	}
	else
	{
		Jump();
	}
}

void ACXFieldPlayerCharacter::ReleaseJumpOrParkour()
{
	bJumpOrParkourInputHeld = false;
	StopJumping();
}

void ACXFieldPlayerCharacter::AlignParkourMontageEntry(UAnimInstance* AnimInstance, UAnimMontage* Montage)
{
	static const FName MovingTraversalCurve(TEXT("MovingTraversal"));
	static const FName LedgeDistanceCurve(TEXT("distance_from_ledge"));
	static const FName FrontLedgeTarget(TEXT("FrontLedge"));

	// 서서 올라가는 동작과 이미 시작 프레임이 선택된 몽타주는 그대로 둔다.
	if (!IsValid(AnimInstance) || !IsValid(Montage)
		|| !Montage->HasCurveData(MovingTraversalCurve)
		|| AnimInstance->Montage_GetPosition(Montage) > KINDA_SMALL_NUMBER)
	{
		return;
	}

	const UMotionWarpingComponent* Warping = FindComponentByClass<UMotionWarpingComponent>();
	const FMotionWarpingTarget* Target = IsValid(Warping) ? Warping->FindWarpTarget(FrontLedgeTarget) : nullptr;
	if (!Target)
	{
		return;
	}

	TArray<FMotionWarpingWindowData> Windows;
	UMotionWarpingUtilities::GetMotionWarpingWindowsForWarpTargetFromAnimation(Montage, FrontLedgeTarget, Windows);
	float SearchEnd = Montage->GetPlayLength();
	for (const FMotionWarpingWindowData& Window : Windows)
	{
		SearchEnd = FMath::Min(SearchEnd, Window.StartTime);
	}
	if (Windows.IsEmpty() || SearchEnd <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	// 첫 워핑/접촉 구간을 건너뛰지 않고 그 직전의 준비 동작 안에서만 선택한다.
	SearchEnd = FMath::Max(0.0f, SearchEnd - 0.001f);
	const float DistanceToLedge = FVector::Dist2D(GetActorLocation(), Target->GetLocation());
	float BestTime = 0.0f;
	float BestError = TNumericLimits<float>::Max();
	constexpr float SampleInterval = 1.0f / 60.0f;
	const int32 SampleCount = FMath::CeilToInt(SearchEnd / SampleInterval);
	for (int32 Sample = 0; Sample <= SampleCount; ++Sample)
	{
		const float Time = FMath::Min(Sample * SampleInterval, SearchEnd);
		float CurveDistance = 0.0f;
		if (UAnimationWarpingLibrary::GetCurveValueFromAnimation(Montage, LedgeDistanceCurve, Time, CurveDistance))
		{
			const float Error = FMath::Abs(FMath::Abs(CurveDistance) - DistanceToLedge);
			if (Error < BestError)
			{
				BestError = Error;
				BestTime = Time;
			}
		}
	}

	if (BestTime > KINDA_SMALL_NUMBER)
	{
		AnimInstance->Montage_SetPosition(Montage, BestTime);
	}
}

void ACXFieldPlayerCharacter::Assassination()
{
	if (ASC && PreemptiveAttackHandle.IsValid() && ASC->HasMatchingGameplayTag(CXVIFieldTag::State_CanAssassination))
	{
		ASC->TryActivateAbility(PreemptiveAttackHandle);

	}
}

void ACXFieldPlayerCharacter::StartUseFlag()
{
	if (!CanStartFlag())
	{
		return;
	}
	ACXExpeditionFlag* Flag = NearByActiveExpeditionFlag.Get();
	if (!IsValid(Flag) || !Flag->CanFlag(this))
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[깃발 휴식] 접근한 깃발: %s"),
		*GetNameSafe(Flag));

	// 임시: 앉기 몽타주 대신 바로 휴식 처리하고 메뉴 태그를 붙인다.
	// 카메라 연출·메뉴가 들어오면 이 자리에서 시작하고, 메뉴가 닫힐 때 태그를 뗀다.
	ActiveExpeditionFlag = Flag;
	ASC->AddLooseGameplayTag(CXVIFieldTag::State_InMenu);
	Flag->CompleteFlag();
}

void ACXFieldPlayerCharacter::OnFlagMontageChanged(UAnimMontage* Montage, FName SectionName, bool bLooped)
{
	if (Montage != FlagMontage || SectionName!=FName(TEXT("SitIdle")) || !bFlagMovementBlocked)
	{
		return;
	}

	// 이번 플래그 처리 완료.
	bFlagMovementBlocked = false;

	if (!IsValid(ASC))
	{
		return;
	}

	ASC->RemoveLooseGameplayTag(CXVIFieldTag::State_MovingBlock);

	// 원정대 깃발로 책임 이전.
	if (ACXExpeditionFlag* Flag = ActiveExpeditionFlag.Get())
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[깃발 휴식] 완료 요청을 받은 깃발: %s"),
			*GetNameSafe(Flag));
		Flag->CompleteFlag();
	}

}

void ACXFieldPlayerCharacter::Attack()
{
	if (IsValid(ASC) && AttackAbilityHandle.IsValid())
	{
		ASC->TryActivateAbility(AttackAbilityHandle);
	}
}

void ACXFieldPlayerCharacter::StartSprint()
{
	if (ASC && SprintAbilityHandle.IsValid())
	{
		ASC->TryActivateAbility(SprintAbilityHandle);
	}
}

void ACXFieldPlayerCharacter::StopSprint()
{
	if (ASC && SprintAbilityHandle.IsValid())
	{
		ASC->CancelAbilityHandle(SprintAbilityHandle);
	}
}

void ACXFieldPlayerCharacter::GrapplingHook()
{
	if (!IsValid(ASC) || !GrapplingHookHandle.IsValid())
	{
		return;
	}

	if (ASC->HasMatchingGameplayTag(CXVIFieldTag::State_CanGrapplingHook)
		&& CurrentGrapplingTarget.IsValid() && CurrentLandingSurface.IsValid())
	{
		ASC->TryActivateAbility(GrapplingHookHandle);
	}
}

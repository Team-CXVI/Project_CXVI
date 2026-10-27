// Fill out your copyright notice in the Description page of Project Settings.

#include "CXFieldPlayerCharacter.h"
#include "InputActionValue.h"
#include "CXVI/Tags/CXFieldTags.h"
#include "CXVI/Character/Field/Interactions/CXExpeditionFlag.h"


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
#include "Components/WidgetComponent.h"
#include "CableComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldPlayerCharacter)

ACXFieldPlayerCharacter::ACXFieldPlayerCharacter()
{
	// 그래플링 UI 설정.
	GrapplingPrompt = CreateDefaultSubobject<UWidgetComponent>(TEXT("GrapplingPrompt"));
	GrapplingPrompt->SetupAttachment(GetRootComponent());
	GrapplingPrompt->SetWidgetSpace(EWidgetSpace::Screen);
	GrapplingPrompt->SetDrawSize(FVector2D(100.0f, 40.0f));
	GrapplingPrompt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GrapplingPrompt->SetVisibility(false);

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

	// 그래플링 태그 여부에 따른 델리게이트 함수 호출/ 후에 콜백을 없애주기위해 변수로 설정.
	GrapplingTagDelegateHandle =
		ASC->RegisterGameplayTagEvent(CXVIFieldTag::State_CanGrapplingHook, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnCanGrapplingHookChanged);

	RefreshGrapplingPrompt();

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

	if (InputMappingContext)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			{
				Subsystem->AddMappingContext(InputMappingContext, 0);
			}
		}
	}

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

		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::StartJumpOrParkour);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::ReleaseJumpOrParkour);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ThisClass::ReleaseJumpOrParkour);
		}
		if (GrapplingAction)
		{
			EnhancedInputComponent->BindAction(GrapplingAction, ETriggerEvent::Started, this, &ThisClass::GrapplingHook);
		}

		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction,ETriggerEvent::Started,this,&ThisClass::StartSprint);

			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ThisClass::StopSprint);

			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ThisClass::StopSprint);
		}

		if (AssassinationAttackAction)
		{
			EnhancedInputComponent->BindAction(AssassinationAttackAction, ETriggerEvent::Started, this, &ThisClass::Assassination);
		}

		if (SaveAction)
		{
			EnhancedInputComponent->BindAction(SaveAction, ETriggerEvent::Started, this, &ThisClass::StartSave);
		}

		if (AttackAction)
		{
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ThisClass::Attack);
		}
	}

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
	if (IsValid(AnimInstance) && IsValid(SaveMontage) &&
		AnimInstance->Montage_IsActive(SaveMontage) &&
		AnimInstance->Montage_GetCurrentSection(SaveMontage) == FName(TEXT("SitIdle")))
	{
		AnimInstance->Montage_Stop(0.2f, SaveMontage);
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

void ACXFieldPlayerCharacter::OnCanGrapplingHookChanged(const FGameplayTag, int32 NewCount)
{
	RefreshGrapplingPrompt();
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

bool ACXFieldPlayerCharacter::CanStartRest() const
{
	if (!IsValid(ASC) || !IsValid(SaveMontage))
	{
		return false;
	}

	// 이미 앉기 처리 중이거나, 다른 행동으로 이동이 제한된 상태.
	if (bSaveMovementBlocked || ASC->HasMatchingGameplayTag(CXVIFieldTag::State_MovingBlock))
	{
		return false;
	}

	if (!ASC->HasMatchingGameplayTag(CXVIFieldTag::State_CanUseExpeditionFlag))
	{
		return false;
	}

	if (!IsValid(GetMesh()))
	{
		return false;
	}

	UAnimInstance* AnimInstace = GetMesh()->GetAnimInstance();
	if (!IsValid(AnimInstace))
	{
		return false;
	}

	// SitIdle에서는 이동 제한이 풀리므로 몽타주 재생 여부도 확인.
	return !AnimInstace->Montage_IsActive(SaveMontage);
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
	if (!IsValid(Movement) || !Movement->IsMovingOnGround()
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

void ACXFieldPlayerCharacter::StartSave()
{
	if (!CanStartRest())
	{
		return;
	}
	ACXExpeditionFlag* Flag = NearByActiveExpeditionFlag.Get();
	if (!IsValid(Flag) || !Flag->CanRest(this))
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[깃발 휴식] 접근한 깃발: %s"),
		*GetNameSafe(Flag));

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!IsValid(AnimInstance))
	{
		return;
	}

	// 재생 실패 시 이동 제한을 추가하지 않음.
	if (PlayAnimMontage(SaveMontage, 1.0f) <= 0.0f)
	{
		return;
	}

	// 재생에 성공했으므로 이번 휴식을 시작한 깃발로 확정.
	ActiveExpeditionFlag = Flag;

	// 재생 후 해당 몽타주 인스턴스에 종료 콜백 연결.
	FOnMontageSectionChanged SectionDelegate;
	SectionDelegate.BindUObject(this, &ThisClass::OnSaveMontageChanged);
	AnimInstance->Montage_SetSectionChangedDelegate(SectionDelegate, SaveMontage);

	bSaveMovementBlocked = true;
	ASC->AddLooseGameplayTag(CXVIFieldTag::State_MovingBlock);
	
}

void ACXFieldPlayerCharacter::OnSaveMontageChanged(UAnimMontage* Montage, FName SectionName, bool bLooped)
{
	if (Montage != SaveMontage || SectionName!=FName(TEXT("SitIdle")) || !bSaveMovementBlocked)
	{
		return;
	}

	// 이번 앉기 처리 완료.
	// SitIdle이 반복돼도 같은 휴식을 중복 처리 하지 않음.
	bSaveMovementBlocked = false;

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
		Flag->CompleteRest();
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

void ACXFieldPlayerCharacter::RefreshGrapplingPrompt()
{
	// 그래플링 프롬프트 리프레시 설정 -> 태그가 들어올때마다 UI 표시 위해.
	if (!GrapplingPrompt)
	{
		return;
	}

	// 그래플링 할 타겟.
	AActor* Target = CurrentGrapplingTarget.Get();
	USceneComponent* TargetComponent = IsValid(Target) ? Target->GetRootComponent() : nullptr;

	APlayerController* PC = Cast<APlayerController>(GetController());

	const bool bShow =
		ASC &&
		ASC->HasMatchingGameplayTag(CXVIFieldTag::State_CanGrapplingHook) &&
		Target &&
		IsValid(TargetComponent) &&
		PC &&
		PC->GetLocalPlayer();

	if (!bShow)
	{
		GrapplingPrompt->SetVisibility(false);
		return;
	}

	// 이 플레이어에게만 표시.
	GrapplingPrompt->SetOwnerPlayer(PC->GetLocalPlayer());

	// UI 위치를 타겟 액터에 붙이기.
	if (GrapplingPrompt->GetAttachParent() != TargetComponent)
	{
		GrapplingPrompt->AttachToComponent(TargetComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	GrapplingPrompt->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));

	const bool bWasVisible = GrapplingPrompt->IsVisible();

	GrapplingPrompt->SetVisibility(true);

	// 숨겨져 있다가 나타는 순간 BP 호출.
	if (!bWasVisible)
	{
		OnGrapplingPromptShown();
	}

}

// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Character/Combat/CXTempFieldCharacterPlayer.h"

#include "CXVI/Character/Combat/CXCharacterPartyData.h"
#include "CXVI/Tags/CXGameplayTags.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXTempFieldCharacterPlayer)

ACXTempFieldCharacterPlayer::ACXTempFieldCharacterPlayer()
{
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 600.f;

	SpringArm->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
}

const TArray<TSubclassOf<ACXCombatCharacterBase>> ACXTempFieldCharacterPlayer::GetCharacterData() const
{
	if (!PartyData)
	{
		return TArray<TSubclassOf<ACXCombatCharacterBase>>();
	}

	if (PartyData->CharacterArray.IsEmpty())
	{
		return TArray<TSubclassOf<ACXCombatCharacterBase>>();
	}

	return PartyData->CharacterArray;
}

void ACXTempFieldCharacterPlayer::BeginPlay()
{
	Super::BeginPlay();

	
}

void ACXTempFieldCharacterPlayer::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (IsValid(PlayerController))
	{
		UEnhancedInputLocalPlayerSubsystem* InputSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());

		if (InputSystem)
		{
			InputSystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ACXTempFieldCharacterPlayer::UnPossessed()
{

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (IsValid(PlayerController))
	{
		UEnhancedInputLocalPlayerSubsystem* InputSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());

		if (InputSystem)
		{
			InputSystem->RemoveMappingContext(DefaultMappingContext);
		}
	}
	Super::UnPossessed();
}

void ACXTempFieldCharacterPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 바인딩 - 입력 액션을 통해서 입력이 전달될 때 실행할 함수 연동.
	// 향상된 입력 컴포넌트로 변환.
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (EnhancedInputComponent)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACXTempFieldCharacterPlayer::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACXTempFieldCharacterPlayer::Look);
	}
}

void ACXTempFieldCharacterPlayer::Move(const FInputActionValue& Value)
{
	// 입력 값 읽어오기 ( 입력에 지정된 타입으로 변환 ).
	FVector2D Movement = Value.Get<FVector2D>();

	// 이동할 방향 만들기.
	// 카메라가 바라보는 방향 (= 컨트롤러가 바라보는 방향)을 기준으로 방향 만들기.

	// 방향을 구하기 위해서는 회전(오리엔테이션)을 먼저 구해야 함.
	// FRotator는 오일러(Euler) 회전을 표기하는데 사용됨.
	// 오일러 회전은 X축으로 몇 도, Y축으로 몇 도, Z축으로 몇 도를 직관적으로 표기.
	FRotator Rotation = GetControlRotation();
	FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	// 앞방향.
	// 회전 행렬을 구하고, 거기에서 앞방향 성분을 추출.
	FVector ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	// 오른쪽 방향.
	FVector RightVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// 무브먼트 컴포넌트에 입력 전달.
	AddMovementInput(ForwardVector, Movement.Y);
	AddMovementInput(RightVector, Movement.X);
}

void ACXTempFieldCharacterPlayer::Look(const FInputActionValue& Value)
{
	// 입력 값 읽어오기 ( 입력에 지정된 타입으로 변환 ).
	FVector2D RotationValue = Value.Get<FVector2D>();

	// 회전 처리 -> 컨트롤러에 회전 입력 전달.
	AddControllerYawInput(RotationValue.X);
	AddControllerPitchInput(RotationValue.Y);
}

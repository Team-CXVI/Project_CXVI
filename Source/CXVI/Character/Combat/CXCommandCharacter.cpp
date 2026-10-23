// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Character/Combat/CXCommandCharacter.h"

#include "CXVI/Character/Combat/CXCombatCharacterPlayer.h"
#include "CXVI/Foundation/Combat/CXCombatManagerSubsystem.h"
#include "CXVI/Actor/Combat/CXTestCombatArena.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputSubsystems.h"
		 
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "InputAction.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCommandCharacter)

// Sets default values
ACXCommandCharacter::ACXCommandCharacter()
{
	/*static ConstructorHelpers::FObjectFinder<UInputMappingContext> FieldRef(
		TEXT("/Game/Input/IMC_Field.IMC_Field")
	);
	if (FieldRef.Succeeded())
	{
		FieldMappingContext = FieldRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> BattleRef(
		TEXT("/Game/Input/IMC_Battle.IMC_Battle")
	);
	if (BattleRef.Succeeded())
	{
		BattleMappingContext = BattleRef.Object;
	}*/
}

void ACXCommandCharacter::AddCommandCharacter(ACXCombatCharacterPlayer* NewPlayer)
{
	CommandCharacters.Add(NewPlayer);
}


void ACXCommandCharacter::UpdateCommandCharacter()
{
	CommandCharacter.Reset();

	for (const auto& WeakPlayer : CommandCharacters)
	{
		ACXCombatCharacterPlayer* CommandPlayer = WeakPlayer.Get();
		if (!CommandPlayer)
		{
			continue;
		}

		if (CommandPlayer->GetIsTurn())
		{
			CommandCharacter = CommandPlayer;
			return;
		}
	}
}

// Called when the game starts or when spawned
void ACXCommandCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ACXCommandCharacter::PossessedBy(AController* NewController)
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

void ACXCommandCharacter::UnPossessed()
{
	

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (IsValid(PlayerController))
	{
		CombatInputMode = ECombatInputMode::Disabled;
		CommandCharacter.Reset();
		CommandCharacters.Reset();

		UEnhancedInputLocalPlayerSubsystem* InputSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());

		if (InputSystem)
		{
			InputSystem->RemoveMappingContext(DefaultMappingContext);
		}
	}
	Super::UnPossessed();
}

// Called to bind functionality to input
void ACXCommandCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (EnhancedInput && BasicAttackAction)
	{
		EnhancedInput->BindAction(BasicAttackAction, ETriggerEvent::Started, this, &ACXCommandCharacter::OnBasicAttackInput);
		EnhancedInput->BindAction(AimModeAction, ETriggerEvent::Started, this, &ACXCommandCharacter::OnAimModeInput);

		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACXCommandCharacter::OnMouseLookInput);
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &ACXCommandCharacter::OnFireInput);
	}

}

void ACXCommandCharacter::OnBasicAttackInput()
{
	if (CombatInputMode != ECombatInputMode::Command)
	{
		return;
	}

	ACXCombatCharacterPlayer* CommandPlayer = CommandCharacter.Get();
	if (!CommandPlayer)
	{
		return;
	}

	CommandPlayer->BasicAttackAbility();
}

void ACXCommandCharacter::OnAimModeInput()
{
	if (CombatInputMode != ECombatInputMode::Command)
	{
		return;
	}

	ACXCombatCharacterPlayer* CommandPlayer = CommandCharacter.Get();
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	UWorld* World = GetWorld();

	if (!IsValid(CommandPlayer) || !IsValid(PlayerController) || !World)
	{
		return;
	}

	UCXCombatManagerSubsystem* CombatManager = World->GetSubsystem<UCXCombatManagerSubsystem>();

	ACXTestCombatArena* Arena = CombatManager ? CombatManager->GetCombatArena() : nullptr;

	if (!IsValid(Arena))
	{
		return;
	}

	if (CommandPlayer->ToggleAimMode())
	{
		//SwitchInputMappintContext(BattleLookMappingContext);
		PlayerController->SetViewTargetWithBlend(CommandPlayer, 0.25f);
	}
	else
	{
		PlayerController->SetViewTargetWithBlend(Arena, 0.25f);
		//AGASGameMode* GameMode = Cast<AGASGameMode>(GetWorld()->GetAuthGameMode());
		//if (GameMode)
		//{
		//	//SwitchInputMappintContext(BattleLookMappingContext);
		//	SetViewTargetWithBlend(GameMode->GetBattleArena(), 0.25f);
		//}
	}
}

void ACXCommandCharacter::OnMouseLookInput(const FInputActionValue& Value)
{
	if (CombatInputMode != ECombatInputMode::Command)
	{
		return;
	}

	ACXCombatCharacterPlayer* CommandPlayer = CommandCharacter.Get();
	if (!CommandPlayer || !CommandPlayer->GetIsAimMode())
	{
		return;
	}

	CommandPlayer->Look(Value);
}

void ACXCommandCharacter::OnFireInput()
{
	if (CombatInputMode != ECombatInputMode::Command)
	{
		return;
	}

	ACXCombatCharacterPlayer* CommandPlayer = CommandCharacter.Get();
	if (!CommandPlayer)
	{
		return;
	}

	CommandPlayer->FireAbility();
}


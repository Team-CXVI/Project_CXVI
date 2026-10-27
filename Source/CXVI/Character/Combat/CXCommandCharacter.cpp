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
#include "UObject/ConstructorHelpers.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCommandCharacter)

// Sets default values
ACXCommandCharacter::ACXCommandCharacter()
{
	/*static ConstructorHelpers::FObjectFinder<UCXCombatInputData> InputDataRef(
		TEXT("/Game/Combat/Input/DA_CombatInputData.DA_CombatInputData"));
	if (InputDataRef.Succeeded())
	{
		CombatInputData = InputDataRef.Object;
	}*/
}

void ACXCommandCharacter::AddCommandCharacter(ACXCombatCharacterPlayer* NewPlayer)
{
	if (!IsValid(NewPlayer))
	{
		return;
	}

	CommandCharacters.AddUnique(NewPlayer);

	NewPlayer->OnTargetSelectionRequested.RemoveAll(this);
	NewPlayer->OnTargetSelectionRequested.AddUObject(
		this,
		&ACXCommandCharacter::HandleTargetSelectionRequested
	);
}

void ACXCommandCharacter::SetCombatInputMode(ECXCombatInputMode NewMode)
{
	const ECXCombatInputMode PreviousMode = CombatInputMode;
	const TWeakObjectPtr<ACXCombatCharacterPlayer> PreviousCharacter = CommandCharacter;
	UpdateCommandCharacter();
	CombatInputMode = NewMode;

	if (NewMode == ECXCombatInputMode::Disabled ||(NewMode == ECXCombatInputMode::Command &&
			(PreviousMode != NewMode || PreviousCharacter != CommandCharacter)))
	{
		TargetCandidates.Reset();
		SelectedTargetIndex = INDEX_NONE;
		MenuState = ECommandMenuState::ActionMenu;
		TargetReturnMenu = ECommandMenuState::ActionMenu;
		SavedSelection.Clear();
	}

}

void ACXCommandCharacter::SetMenuState(ECommandMenuState NewState)
{
	if (MenuState == ECommandMenuState::ActionMenu && NewState != MenuState)
	{
		if (ACXCombatCharacterPlayer* Player = CommandCharacter.Get();
			IsValid(Player) && Player->GetIsAimMode())
		{
			OnAimModeInput();
		}
	}

	MenuState = NewState;
}




// Called when the game starts or when spawned
void ACXCommandCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ACXCommandCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	InputSubsystem.Reset();
	Super::EndPlay(EndPlayReason);
}

void ACXCommandCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InputSubsystem.Reset();
	APlayerController* PlayerController = Cast<APlayerController>(NewController);
	if (IsValid(PlayerController))
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
			if (InputSubsystem.IsValid() && ActionMappingContext && ReactionMappingContext)
			{
				FModifyContextOptions Options;
				Options.bIgnoreAllPressedKeysUntilRelease = true;

				InputSubsystem->AddMappingContext(ActionMappingContext, 0, Options);
				InputSubsystem->AddMappingContext(ReactionMappingContext, 0, Options);
			}
		}
	}
}

void ACXCommandCharacter::UnPossessed()
{
	RemoveInputMappingContexts();
	InputSubsystem.Reset();
	CombatInputMode = ECXCombatInputMode::Disabled;
	MenuState = ECommandMenuState::ActionMenu;
	TargetReturnMenu = ECommandMenuState::ActionMenu;
	SavedSelection.Clear();

	for (const auto& WeakPlayer : CommandCharacters)
	{
		if (ACXCombatCharacterPlayer* Player = WeakPlayer.Get())
		{
			Player->OnTargetSelectionRequested.RemoveAll(this);
		}
	}

	TargetCandidates.Reset();
	SelectedTargetIndex = INDEX_NONE;

	CommandCharacter.Reset();
	CommandCharacters.Reset();
	Super::UnPossessed();
}

// Called to bind functionality to input
void ACXCommandCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!EnhancedInput)
	{
		return;
	}

	EnhancedInput->BindAction(ConfirmAction, ETriggerEvent::Started, this, &ACXCommandCharacter::HandleInputConfirm);
	EnhancedInput->BindAction(CancelAction, ETriggerEvent::Started, this, &ACXCommandCharacter::HandleInputCancel);
	EnhancedInput->BindAction(Slot1Action, ETriggerEvent::Started, this, &ACXCommandCharacter::HandleInputSlot1);
	EnhancedInput->BindAction(Slot2Action, ETriggerEvent::Started, this, &ACXCommandCharacter::HandleInputSlot2);
	EnhancedInput->BindAction(Slot3Action, ETriggerEvent::Started, this, &ACXCommandCharacter::HandleInputSlot3);
	EnhancedInput->BindAction(ParryAction, ETriggerEvent::Started, this, &ACXCommandCharacter::HandleInputParry);
	EnhancedInput->BindAction(DodgeAction, ETriggerEvent::Started, this, &ACXCommandCharacter::HandleInputDodge);
	EnhancedInput->BindAction(LeftSelectAction, ETriggerEvent::Started, this, &ACXCommandCharacter::HandleInputSelectLeft);
	EnhancedInput->BindAction(RightSelectAction, ETriggerEvent::Started, this, &ACXCommandCharacter::HandleInputSelectRight);
	EnhancedInput->BindAction(ToggleAimModeAction, ETriggerEvent::Started, this, &ACXCommandCharacter::HandleInputToggleAimMode);
	EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &ACXCommandCharacter::HandleInputFire);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACXCommandCharacter::HandleInputLook);
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


void ACXCommandCharacter::RemoveInputMappingContexts()
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = InputSubsystem.Get())
	{
		if (ActionMappingContext)
		{
			Subsystem->RemoveMappingContext(ActionMappingContext.Get());
		}

		if (ReactionMappingContext)
		{
			Subsystem->RemoveMappingContext(ReactionMappingContext.Get());
		}
	}
}

void ACXCommandCharacter::HandleInputConfirm()
{
	if (CombatInputMode != ECXCombatInputMode::Command)
	{
		return;
	}

	switch (MenuState)
	{
	case ECommandMenuState::ActionMenu:
	{
		SavedSelection.Clear();
		SavedSelection.Type = ECXActionType::BasicAttack;
		SavedSelection.TargetTeam = ECXTargetTeam::Enemy;

		SubmitActionToPlayer(SavedSelection);
		break;
	}
	case ECommandMenuState::TargetSelect:
	{
		ACXCombatCharacterPlayer* Player = CommandCharacter.Get();

		if (!IsValid(Player) || !TargetCandidates.IsValidIndex(SelectedTargetIndex))
		{
			return;
		}

		// 대상 선택중 마나나 사용중 조건이 바뀔수도 있으니 재검사.

		if (SavedSelection.Type == ECXActionType::Skill &&
			!Player->CanActivateSelectSkill(SavedSelection.SlotIndex))
		{
			return;
		}

		Player->SubmitTarget(TargetCandidates[SelectedTargetIndex].Get());
		break;
	}
	}
}

void ACXCommandCharacter::HandleInputCancel()
{
	if (CombatInputMode != ECXCombatInputMode::Command)
	{
		return;
	}

	switch (MenuState)
	{
	case ECommandMenuState::SkillMenu:
	{
		UE_LOG(LogTemp, Display, TEXT("메뉴로 돌아가기"));
		SavedSelection.Clear();
		SetMenuState(ECommandMenuState::ActionMenu);
		return;
	}
	case ECommandMenuState::TargetSelect:
	{
		CancelTargetSelection();
		return;
	}
	case ECommandMenuState::ItemMenu:
	{
		UE_LOG(LogTemp, Display, TEXT("메뉴로 돌아가기"));
		SavedSelection.Clear();
		SetMenuState(ECommandMenuState::ActionMenu);
		return;
	}
	}
}

void ACXCommandCharacter::HandleInputSlot1()
{
	if (CombatInputMode != ECXCombatInputMode::Command)
	{
		return;
	}

	switch (MenuState)
	{
	case ECommandMenuState::SkillMenu:
	{
		UE_LOG(LogTemp, Display, TEXT("1번 스킬 선택"));
		SavedSelection.Clear();
		SavedSelection.Type = ECXActionType::Skill;
		SavedSelection.SlotIndex = 0;
		SavedSelection.TargetTeam = ECXTargetTeam::Enemy;

		SubmitActionToPlayer(SavedSelection);
		return;
	}
	case ECommandMenuState::ItemMenu:
	{
		UE_LOG(LogTemp, Display, TEXT("1번아이템 선택"));
		return;
	}
	}
}

void ACXCommandCharacter::HandleInputSlot2()
{
	if (CombatInputMode != ECXCombatInputMode::Command)
	{
		return;
	}

	switch (MenuState)
	{
	case ECommandMenuState::SkillMenu:
	{
		UE_LOG(LogTemp, Display, TEXT("2번 스킬 선택"));
		SavedSelection.Clear();
		SavedSelection.Type = ECXActionType::Skill;
		SavedSelection.SlotIndex = 1;
		SavedSelection.TargetTeam = ECXTargetTeam::Enemy;
		SubmitActionToPlayer(SavedSelection);

		return;
	}
	case ECommandMenuState::ActionMenu:
	{
		UE_LOG(LogTemp, Display, TEXT("아이템선택창"));
		SetMenuState(ECommandMenuState::ItemMenu);
		return;
	}
	case ECommandMenuState::ItemMenu:
	{
		UE_LOG(LogTemp, Display, TEXT("2번 아이템 선택"));
		
		return;
	}
	}
}

void ACXCommandCharacter::HandleInputSlot3()
{
	if (CombatInputMode != ECXCombatInputMode::Command)
	{
		return;
	}

	switch (MenuState)
	{
	case ECommandMenuState::SkillMenu:
	{
		UE_LOG(LogTemp, Display, TEXT("3번 스킬 선택"));
		SavedSelection.Clear();
		SavedSelection.Type = ECXActionType::Skill;
		SavedSelection.SlotIndex = 2;
		SavedSelection.TargetTeam = ECXTargetTeam::Enemy;
		SubmitActionToPlayer(SavedSelection);
		return;
	}
	case ECommandMenuState::ActionMenu:
	{
		UE_LOG(LogTemp, Display, TEXT("스킬선택창"));
		SetMenuState(ECommandMenuState::SkillMenu);
		return;
	}
	case ECommandMenuState::ItemMenu:
	{
		UE_LOG(LogTemp, Display, TEXT("3번 아이템 선택"));

		return;
	}
	}
}

void ACXCommandCharacter::HandleInputSelectLeft()
{
	if (CombatInputMode != ECXCombatInputMode::Command ||
		MenuState != ECommandMenuState::TargetSelect)
	{
		return;
	}

	MoveTargetSelection(-1);
}

void ACXCommandCharacter::HandleInputSelectRight()
{
	if (CombatInputMode != ECXCombatInputMode::Command ||
		MenuState != ECommandMenuState::TargetSelect)
	{
		return;
	}

	MoveTargetSelection(1);
}

void ACXCommandCharacter::HandleInputParry()
{
	if (CombatInputMode != ECXCombatInputMode::Reaction)
	{
		return;
	}
	UE_LOG(LogTemp, Display, TEXT("패리 키 누름 "));
}

void ACXCommandCharacter::HandleInputDodge()
{
	if (CombatInputMode != ECXCombatInputMode::Reaction)
	{
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("회피 키 누름 "));
}

void ACXCommandCharacter::HandleInputFire()
{
	if (CombatInputMode != ECXCombatInputMode::Command
		|| MenuState != ECommandMenuState::ActionMenu)
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

void ACXCommandCharacter::HandleInputToggleAimMode()
{
	if (CombatInputMode != ECXCombatInputMode::Command
		|| MenuState != ECommandMenuState::ActionMenu)
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

void ACXCommandCharacter::HandleInputLook(const FInputActionValue& Value )
{
	if (CombatInputMode != ECXCombatInputMode::Command
		|| MenuState != ECommandMenuState::ActionMenu)
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


void ACXCommandCharacter::SubmitActionToPlayer(const FCXActionSelection& Selection)
{
	ACXCombatCharacterPlayer* Player = CommandCharacter.Get();
	if (!Player)
	{
		return;
	}

	// 부족하면 캐릭터에 행동 제출하지 않음.
	// 메뉴와 턴페이즈를 그대로 유지.
	if (Selection.Type == ECXActionType::Skill && !Player->CanActivateSelectSkill(Selection.SlotIndex))
	{
		return;
	}

	// SubmitAction 내부의 델리게이트가 동기 실행될 수 있어 전송 전에 메뉴를 준비한다.
	TargetReturnMenu = MenuState;
	Player->SubmitAction(Selection);
}


void ACXCommandCharacter::OnAimModeInput()
{
	if (CombatInputMode != ECXCombatInputMode::Command)
	{
		return;
	}

	if (MenuState != ECommandMenuState::ActionMenu)
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
	if (CombatInputMode != ECXCombatInputMode::Command)
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
	if (CombatInputMode != ECXCombatInputMode::Command)
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

void ACXCommandCharacter::HandleTargetSelectionRequested(ACXCombatCharacterPlayer* Sender, const TArray<TWeakObjectPtr<APawn>>& Candidates)
{
	if (!IsValid(Sender) || Sender != CommandCharacter.Get())
	{
		return;
	}

	TargetCandidates.Reset();

	for (const auto& Candidate : Candidates)
	{
		ACXCombatCharacterBase* Target = Cast<ACXCombatCharacterBase>(Candidate.Get());

		if (IsValid(Target) && Target->GetIsCharacterAlive())
		{
			TargetCandidates.Add(Candidate);
		}
	}

	SelectedTargetIndex = TargetCandidates.IsEmpty() ? INDEX_NONE : 0;

	if (TargetCandidates.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("선택 가능한 대상이 없습니다."));
		CancelTargetSelection();
		return;
	}

	SetMenuState(ECommandMenuState::TargetSelect);

	UE_LOG(LogTemp, Display, TEXT("선택 대상: %s"),
		*GetNameSafe(TargetCandidates[SelectedTargetIndex].Get()));
}

void ACXCommandCharacter::MoveTargetSelection(int32 Direction)
{
	const int32 Count = TargetCandidates.Num();
	if (Count == 0)
	{
		return;
	}

	for (int32 Attempt = 0; Attempt < Count; ++Attempt)
	{
		SelectedTargetIndex = (SelectedTargetIndex + Direction + Count) % Count;

		ACXCombatCharacterBase* Target = Cast<ACXCombatCharacterBase>(TargetCandidates[SelectedTargetIndex].Get());

		if (IsValid(Target) && Target->GetIsCharacterAlive())
		{
			UE_LOG(LogTemp, Display, TEXT("선택 대상: %s"), *Target->GetName());

			// 이전 대상 표시 해제 → 현재 대상 표시
			return;
		}
	}

	CancelTargetSelection();
}

void ACXCommandCharacter::CancelTargetSelection()
{
	ACXCombatCharacterPlayer* Player = CommandCharacter.Get();

	TargetCandidates.Reset();
	SelectedTargetIndex = INDEX_NONE;
	SavedSelection.Clear();

	// 기본 공격이었다면 행동 메뉴, 스킬이었다면 스킬 메뉴.
	SetMenuState(TargetReturnMenu);

	if (IsValid(Player))
	{
		Player->CancelSelection();
	}
}


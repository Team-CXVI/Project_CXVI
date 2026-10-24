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
	static ConstructorHelpers::FObjectFinder<UCXCombatInputData> InputDataRef(
		TEXT("/Game/Combat/Input/DA_CombatInputData.DA_CombatInputData"));
	if (InputDataRef.Succeeded())
	{
		CombatInputData = InputDataRef.Object;
	}
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

void ACXCommandCharacter::SetCombatInputMode(ECombatInputMode NewMode)
{
	const ECombatInputMode PreviousMode = CombatInputMode;
	const TWeakObjectPtr<ACXCombatCharacterPlayer> PreviousCharacter = CommandCharacter;
	UpdateCommandCharacter();
	CombatInputMode = NewMode;

	if (NewMode == ECombatInputMode::Disabled ||(NewMode == ECombatInputMode::Command &&
			(PreviousMode != NewMode || PreviousCharacter != CommandCharacter)))
	{
		TargetCandidates.Reset();
		SelectedTargetIndex = INDEX_NONE;
		MenuState = ECommandMenuState::ActionMenu;
		TargetReturnMenu = ECommandMenuState::ActionMenu;
		SavedSelection.Clear();
	}

	RefreshInputMappingContexts();
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
	RefreshInputMappingContexts();
}

void ACXCommandCharacter::RefreshInputMappingContexts()
{
	UEnhancedInputLocalPlayerSubsystem* Subsystem = InputSubsystem.Get();
	if (!Subsystem)
	{
		return;
	}

	ECXInputContext Context = ECXInputContext::Unassigned;
	if (CombatInputMode == ECombatInputMode::Command)
	{
		switch (MenuState)
		{
		case ECommandMenuState::ActionMenu: Context = ECXInputContext::ActionMenu; break;
		case ECommandMenuState::ItemMenu: Context = ECXInputContext::ItemMenu; break;
		case ECommandMenuState::SkillMenu: Context = ECXInputContext::SkillMenu; break;
		case ECommandMenuState::TargetSelect: Context = ECXInputContext::TargetSelect; break;
		}
	}
	else if (CombatInputMode == ECombatInputMode::Reaction)
	{
		Context = ECXInputContext::Reaction;
	}

	TArray<TWeakObjectPtr<UInputMappingContext>> DesiredContexts;
	if (CombatInputData && CombatInputMode != ECombatInputMode::Disabled)
	{
		for (const FInputData& Data : CombatInputData->GetInputData())
		{
			const bool bCommon = Data.Context == ECXInputContext::Common &&
				CombatInputMode == ECombatInputMode::Command;
			if (Data.IMC && (bCommon || Data.Context == Context))
			{
				DesiredContexts.AddUnique(Data.IMC.Get());
			}
		}
	}

	FModifyContextOptions Options;
	Options.bIgnoreAllPressedKeysUntilRelease = true;
	for (const auto& ActiveContext : ActiveMappingContexts)
	{
		if (ActiveContext.IsValid() && !DesiredContexts.Contains(ActiveContext))
		{
			Subsystem->RemoveMappingContext(ActiveContext.Get(), Options);
		}
	}
	for (const auto& DesiredContext : DesiredContexts)
	{
		if (!ActiveMappingContexts.Contains(DesiredContext))
		{
			const bool bCommon = CombatInputData->GetInputData().ContainsByPredicate(
				[&DesiredContext](const FInputData& Data)
				{
					return Data.IMC.Get() == DesiredContext.Get() && Data.Context == ECXInputContext::Common;
				});
			Subsystem->AddMappingContext(DesiredContext.Get(), bCommon ? 0 : 1, Options);
		}
	}
	ActiveMappingContexts = MoveTemp(DesiredContexts);
}

void ACXCommandCharacter::RemoveInputMappingContexts()
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = InputSubsystem.Get())
	{
		for (const auto& Context : ActiveMappingContexts)
		{
			if (Context.IsValid())
			{
				Subsystem->RemoveMappingContext(Context.Get());
			}
		}
	}
	ActiveMappingContexts.Reset();
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

void ACXCommandCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveInputMappingContexts();
	InputSubsystem.Reset();
	Super::EndPlay(EndPlayReason);
}

void ACXCommandCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	RemoveInputMappingContexts();
	InputSubsystem.Reset();
	APlayerController* PlayerController = Cast<APlayerController>(NewController);
	if (IsValid(PlayerController))
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
			RefreshInputMappingContexts();
		}
	}
}

void ACXCommandCharacter::UnPossessed()
{
	RemoveInputMappingContexts();
	InputSubsystem.Reset();
	CombatInputMode = ECombatInputMode::Disabled;
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

	if (!EnhancedInput || !CombatInputData)
	{
		return;
	}

	TMap<const UInputAction*, TSet<ETriggerEvent>> BoundEvents;
	for (const FInputData& ModeData : CombatInputData->GetInputData())
	{
		if (!ModeData.IMC || ModeData.Context == ECXInputContext::Unassigned)
		{
			UE_LOG(LogTemp, Warning, TEXT("입력 데이터의 IMC와 Context를 설정해주세요."));
			continue;
		}
		for (const FCXInputActionData& Data : ModeData.Actions)
		{
			const UInputAction* Action = Data.Action.Get();

			if (!Action || BoundEvents.FindOrAdd(Action).Contains(Data.TriggerEvent))
			{
				continue;
			}

			EnhancedInput->BindAction(
				Action,
				Data.TriggerEvent,
				this,
				&ACXCommandCharacter::HandleInputAction,
				Data.TriggerEvent
			);
			BoundEvents.FindChecked(Action).Add(Data.TriggerEvent);
		}
	}

}


void ACXCommandCharacter::HandleMenuInput(ECommandKey Key)
{
	if (CombatInputMode != ECombatInputMode::Command)
	{
		return;
	}

	switch (MenuState)
	{
	case ECommandMenuState::ActionMenu:
		HandleActionMenuInput(Key);
		break;

	case ECommandMenuState::ItemMenu:
		HandleItemMenuInput(Key);
		break;

	case ECommandMenuState::SkillMenu:
		HandleSkillMenuInput(Key);
		break;

	case ECommandMenuState::TargetSelect:
		HandleTargetSelectInput(Key);
		break;
	}
}

void ACXCommandCharacter::HandleActionMenuInput(ECommandKey Key)
{
	// 메뉴 선택
	switch (Key)
	{
		// w - 아이템
	case ECommandKey::W:
		UE_LOG(LogTemp, Display, TEXT("아이템선택창"));
		SetMenuState(ECommandMenuState::ItemMenu);
		break;
		// e - 스킬
	case ECommandKey::E:
		UE_LOG(LogTemp, Display, TEXT("스킬선택창"));
		SetMenuState(ECommandMenuState::SkillMenu);
		break;
		// f - 기본공격
	case ECommandKey::F:
		SavedSelection.Clear();
		SavedSelection.Type = ECXActionType::BasicAttack;
		SavedSelection.TargetTeam = ECXTargetTeam::Enemy;

		SubmitActionToPlayer(SavedSelection);
		break;
	}

}

void ACXCommandCharacter::HandleItemMenuInput(ECommandKey Key)
{
	// 아이템 고르는 창
	// q w e로 선택 가능 esc로 뒤로가기
	switch (Key)
	{
	//case ECommandKey::Q:
	//	UE_LOG(LogTemp, Display, TEXT("1번 아이템 선택"));
	//	/*Selection.Clear();
	//	Selection.Type = ECXActionType::Item;
	//	Selection.TargetTeam = ECXTargetTeam::Player;

	//	SubmitActionToPlayer(Selection);*/
	//	break;
	//case ECommandKey::W:
	//	UE_LOG(LogTemp, Display, TEXT("1번 아이템 선택"));
	//	break;
	//case ECommandKey::E:
	//	UE_LOG(LogTemp, Display, TEXT("1번 아이템 선택"));
	//	break;
	case ECommandKey::ESC:
		UE_LOG(LogTemp, Display, TEXT("메뉴로 돌아가기"));
		SavedSelection.Clear();
		SetMenuState(ECommandMenuState::ActionMenu);
		break;
	}
}

void ACXCommandCharacter::HandleSkillMenuInput(ECommandKey Key)
{
	// 스킬 고르는 창
	// q w e로 선택 하고 esc로 뒤로 가기
	switch (Key)
	{
	case ECommandKey::Q:
		UE_LOG(LogTemp, Display, TEXT("1번 스킬 선택"));
		SavedSelection.Clear();
		SavedSelection.Type = ECXActionType::Skill;
		SavedSelection.SlotIndex = 0;
		SavedSelection.TargetTeam = ECXTargetTeam::Enemy;

		SubmitActionToPlayer(SavedSelection);
		break;
	case ECommandKey::W:
		UE_LOG(LogTemp, Display, TEXT("2번 스킬 선택"));
		SavedSelection.Clear();
		SavedSelection.Type = ECXActionType::Skill;
		SavedSelection.SlotIndex = 1;
		SavedSelection.TargetTeam = ECXTargetTeam::Enemy;

		SubmitActionToPlayer(SavedSelection);
		break;
	case ECommandKey::E:
		UE_LOG(LogTemp, Display, TEXT("3번 스킬 선택"));
		SavedSelection.Clear();
		SavedSelection.Type = ECXActionType::Skill;
		SavedSelection.SlotIndex = 2;
		SavedSelection.TargetTeam = ECXTargetTeam::Enemy;

		SubmitActionToPlayer(SavedSelection);
		break;
	case ECommandKey::ESC:
		UE_LOG(LogTemp, Display, TEXT("메뉴로 돌아가기"));
		SavedSelection.Clear();
		SetMenuState(ECommandMenuState::ActionMenu);
		break;
	}
}

void ACXCommandCharacter::HandleTargetSelectInput(ECommandKey Key)
{
	switch (Key)
	{
	case ECommandKey::A:
		MoveTargetSelection(-1);
		return;

	case ECommandKey::D:
		MoveTargetSelection(1);
		return;

	case ECommandKey::F:
	{
		ACXCombatCharacterPlayer* Player = CommandCharacter.Get();

		if (IsValid(Player) && TargetCandidates.IsValidIndex(SelectedTargetIndex))
		{
			Player->SubmitTarget(TargetCandidates[SelectedTargetIndex].Get());
		}

		return;
	}
	case ECommandKey::ESC:
		CancelTargetSelection();
		return;
	default:
		return;
	}
}

void ACXCommandCharacter::SubmitActionToPlayer(const FCXActionSelection& Selection)
{
	ACXCombatCharacterPlayer* Player = CommandCharacter.Get();
	if (!Player)
	{
		return;
	}

	// SubmitAction 내부의 델리게이트가 동기 실행될 수 있어 전송 전에 메뉴를 준비한다.
	TargetReturnMenu = MenuState;
	Player->SubmitAction(Selection);
}

void ACXCommandCharacter::HandleInputAction(const FInputActionInstance& Instance, ETriggerEvent TriggerEvent)
{
	if (!CombatInputData || CombatInputMode == ECombatInputMode::Disabled)
	{
		return;
	}

	// IA/이벤트마다 한 번만 수신하고, 메뉴 전용 명령을 공통 명령보다 우선한다.
	// 처리 도중 IMC를 바꿔도 같은 입력으로 다음 메뉴의 명령이 연속 실행되지 않는다.
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		for (const FInputData& ModeData : CombatInputData->GetInputData())
		{
			const bool bCommon = ModeData.Context == ECXInputContext::Common;
			if (bCommon != (Pass == 1) || !ActiveMappingContexts.Contains(ModeData.IMC.Get()))
			{
				continue;
			}

			for (const FCXInputActionData& Data : ModeData.Actions)
			{
				if (Data.Action.Get() == Instance.GetSourceAction().Get() && Data.TriggerEvent == TriggerEvent)
				{
					HandleInputCommand(Instance.GetValue(), Data.Command);
					return;
				}
			}
		}
	}
}

void ACXCommandCharacter::HandleInputCommand(const FInputActionValue& Value,
	ECXInputCommand Command)
{
	if (CombatInputMode != ECombatInputMode::Command)
	{
		return;
	}

	switch (Command)
	{
	case ECXInputCommand::OpenItemMenu:
		if (MenuState == ECommandMenuState::ActionMenu)
		{
			HandleActionMenuInput(ECommandKey::W);
		}
		break;

	case ECXInputCommand::OpenSkillMenu:
		if (MenuState == ECommandMenuState::ActionMenu)
		{
			HandleActionMenuInput(ECommandKey::E);
		}
		break;

	case ECXInputCommand::BasicAttack:
		if (MenuState == ECommandMenuState::ActionMenu)
		{
			HandleActionMenuInput(ECommandKey::F);
		}
		break;

	case ECXInputCommand::Skill1:
	case ECXInputCommand::Skill2:
	case ECXInputCommand::Skill3:
		if (MenuState == ECommandMenuState::SkillMenu)
		{
			const ECommandKey Key = Command == ECXInputCommand::Skill1 ? ECommandKey::Q :
				Command == ECXInputCommand::Skill2 ? ECommandKey::W : ECommandKey::E;
			HandleSkillMenuInput(Key);
		}
		break;

	case ECXInputCommand::PreviousTarget:
	case ECXInputCommand::NextTarget:
		if (MenuState == ECommandMenuState::TargetSelect)
		{
			HandleTargetSelectInput(Command == ECXInputCommand::PreviousTarget ?
				ECommandKey::A : ECommandKey::D);
		}
		break;

	case ECXInputCommand::Confirm:
		if (MenuState == ECommandMenuState::TargetSelect)
		{
			HandleTargetSelectInput(ECommandKey::F);
		}
		break;

	case ECXInputCommand::Cancel:
		HandleMenuInput(ECommandKey::ESC);
		break;

	case ECXInputCommand::ToggleAim:
		OnAimModeInput();
		break;

	case ECXInputCommand::Look:
		OnMouseLookInput(Value);
		break;

	case ECXInputCommand::Fire:
		if (MenuState == ECommandMenuState::ActionMenu)
		{
			OnFireInput();
		}
		break;

	default:
		break;
	}
}

void ACXCommandCharacter::OnAimModeInput()
{
	if (CombatInputMode != ECombatInputMode::Command)
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


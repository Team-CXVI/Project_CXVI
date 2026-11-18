// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Character/Combat/CXCommandCharacter.h"

#include "CXVI/Character/Combat/CXCombatCharacterPlayer.h"
#include "CXVI/Foundation/Combat/CXCombatManagerSubsystem.h"

#include "Character/Combat/Components/CXJustTimingComponent.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/PlayerCameraManager.h"

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

		if (NewMode == ECXCombatInputMode::Command)
		{
			RefreshMenuCamera();
		}
	}

	RefreshInputModeTag();

	NotifyCommandAvailability();
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
	RefreshInputModeTag();
	RefreshMenuCamera();
	NotifyCommandAvailability();
}

void ACXCommandCharacter::RefreshMenuCamera() const
{
	const ACXCombatCharacterBase* CameraSubject = CommandCharacter.Get();
	if (MenuState == ECommandMenuState::TargetSelect)
	{
		if (!TargetCandidates.IsValidIndex(SelectedTargetIndex))
		{
			return;
		}
		CameraSubject = Cast<ACXCombatCharacterBase>(TargetCandidates[SelectedTargetIndex].Get());
	}

	if (const UCXCombatManagerSubsystem* CombatManager = UCXCombatManagerSubsystem::TryGet(this))
	{
		CombatManager->RequestMenuCamera(MenuState, CameraSubject);
	}
}

// Called when the game starts or when spawned
void ACXCommandCharacter::BeginPlay()
{
	Super::BeginPlay();

	CommandRequestListener = UCXGameplayMessageSubsystem::Get(this).RegisterListener(CXVIGameplayTags::Message_Combat_CommandRequest, this, &ThisClass::HandleCommandRequest);
}

void ACXCommandCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CommandRequestListener.Unregister();

	Super::EndPlay(EndPlayReason);
}

void ACXCommandCharacter::UnPossessed()
{
	CombatInputMode = ECXCombatInputMode::Disabled;
	MenuState = ECommandMenuState::ActionMenu;
	TargetReturnMenu = ECommandMenuState::ActionMenu;
	SavedSelection.Clear();

	// 남아 있는 입력 모드 태그 반납
	RefreshInputModeTag();

	NotifyCommandAvailability();

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
	
	EnhancedInput->BindAction(JustTimingAction, ETriggerEvent::Started, this, &ACXCommandCharacter::HandleInputJustTiming);

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


FGameplayTag ACXCommandCharacter::FindInputModeTagForCurrentState() const
{
	switch (CombatInputMode)
	{
	case ECXCombatInputMode::Command:
		return MenuState == ECommandMenuState::TargetSelect ? CXVIGameplayTags::InputMode_Combat_TargetSelect : CXVIGameplayTags::InputMode_Combat_Command;

	case ECXCombatInputMode::JustTiming:
		return CXVIGameplayTags::InputMode_Combat_JustTiming;

	case ECXCombatInputMode::Reaction:
		return CXVIGameplayTags::InputMode_Combat_Reaction;

	default:
		return FGameplayTag();
	}
}

void ACXCommandCharacter::RefreshInputModeTag()
{
	const FGameplayTag DesiredTag = FindInputModeTagForCurrentState();
	if (DesiredTag == AppliedInputModeTag)
	{
		return;
	}

	UCXInputControlSubsystem* ControlSystem = UCXInputControlSubsystem::TryGet(GetWorld());
	if (nullptr == ControlSystem)
	{
		return;
	}

	// 태그가 바뀌면 다음 처리 때 매핑이 재구성됨. 아직 눌려 있는 키는 새 매핑에서 뗄 때까지 무시
	ControlSystem->RemoveInputModeTag(InputModeHandle);
	AppliedInputModeTag = FGameplayTag();

	if (!DesiredTag.IsValid())
	{
		return;
	}

	InputModeHandle = ControlSystem->AddInputModeTag(DesiredTag, TEXT("CombatInputMode"));

	// 요청이 거부되면 다음 상태 변경 때 다시 시도
	if (InputModeHandle.IsValid())
	{
		AppliedInputModeTag = DesiredTag;
	}
}

void ACXCommandCharacter::HandleInputConfirm()
{
	if (CombatInputMode != ECXCombatInputMode::Command)
	{
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("[전투] 확인키 누름"));

	switch (MenuState)
	{
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

		if (SavedSelection.Type == ECXActionType::Item)
		{
			ACXCombatCharacterBase* Target = Cast<ACXCombatCharacterBase>(TargetCandidates[SelectedTargetIndex].Get());

			Player->UseItem(SavedSelection.ItemId, Target);
			return;
		}

		Player->SubmitTarget(TargetCandidates[SelectedTargetIndex].Get());

		if (const UCXCombatManagerSubsystem* CombatManager = UCXCombatManagerSubsystem::TryGet(this))
		{
			CombatManager->RequestCameraPreset(TEXT("Attack.Begin"), Player);
		}
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
	const ACXCombatCharacterPlayer* AimingPlayer = CommandCharacter.Get();

	if (CombatInputMode != ECXCombatInputMode::Command ||
		(IsValid(AimingPlayer) && AimingPlayer->GetIsAimMode()))
	{
		return;
	}

	switch (MenuState)
	{
	case ECommandMenuState::ActionMenu:
	{
		UE_LOG(LogTemp, Display, TEXT("아이템선택창"));
		SetMenuState(ECommandMenuState::ItemMenu);
		return;
	}
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
		SelectItemSlot(0);
		return;
	}
	}
}

void ACXCommandCharacter::HandleInputSlot2()
{
	const ACXCombatCharacterPlayer* AimingPlayer = CommandCharacter.Get();

	if (CombatInputMode != ECXCombatInputMode::Command ||
		(IsValid(AimingPlayer) && AimingPlayer->GetIsAimMode()))
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
		UE_LOG(LogTemp, Display, TEXT("스킬선택창"));
		SetMenuState(ECommandMenuState::SkillMenu);
		return;
	}
	case ECommandMenuState::ItemMenu:
	{
		SelectItemSlot(1);
		return;
	}
	}
}

void ACXCommandCharacter::HandleInputSlot3()
{
	const ACXCombatCharacterPlayer* AimingPlayer = CommandCharacter.Get();

	if (CombatInputMode != ECXCombatInputMode::Command ||
		(IsValid(AimingPlayer) && AimingPlayer->GetIsAimMode()))
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
		SavedSelection.Clear();
		SavedSelection.Type = ECXActionType::BasicAttack;
		SavedSelection.TargetTeam = ECXTargetTeam::Enemy;

		SubmitActionToPlayer(SavedSelection);
		return;
	}
	case ECommandMenuState::ItemMenu:
	{
		SelectItemSlot(2);
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

	for (int32 Index = 0; Index < CommandCharacters.Num(); ++Index)
	{
		ACXCombatCharacterPlayer* CommandPlayer = CommandCharacters[Index].Get();
		if (!IsValid(CommandPlayer) || !CommandPlayer->GetIsCharacterAlive())
		{
			continue;
		}

		if (CommandPlayer->GetIsDefenseTarget())
		{
			CommandPlayer->ParryAbility();
		}
	}
}

void ACXCommandCharacter::HandleInputDodge()
{
	if (CombatInputMode != ECXCombatInputMode::Reaction)
	{
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("회피 키 누름 "));

	for (int32 Index = 0; Index < CommandCharacters.Num(); ++Index)
	{
		ACXCombatCharacterPlayer* CommandPlayer = CommandCharacters[Index].Get();
		if (!IsValid(CommandPlayer) || !CommandPlayer->GetIsCharacterAlive())
		{
			continue;
		}

		if (CommandPlayer->GetIsDefenseTarget())
		{
			CommandPlayer->DodgeAbility();
		}
	}


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
	const APlayerController* PC = Cast<APlayerController>(GetController());
	const APlayerCameraManager* CameraManager =
		PC ? PC->PlayerCameraManager.Get() : nullptr;

	if (CameraManager && CameraManager->PendingViewTarget.Target)
	{
		return;
	}

	OnAimModeInput();
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

void ACXCommandCharacter::HandleInputFlee()
{
	ACXCombatCharacterPlayer* CommandPlayer = CommandCharacter.Get();

	if (CombatInputMode != ECXCombatInputMode::Command ||
		MenuState == ECommandMenuState::TargetSelect ||
		!IsValid(CommandPlayer) ||
		CommandPlayer->GetIsAimMode())
	{
		return;
	}

	// 보스전에서는 도망 불가
	const UCXCombatManagerSubsystem* CombatManager = UCXCombatManagerSubsystem::TryGet(this);
	if (!CombatManager || !CombatManager->CanFlee())
	{
		return;
	}

	CommandPlayer->TryRunaway();
}

void ACXCommandCharacter::HandleInputSkipTurn()
{
	ACXCombatCharacterPlayer* CommandPlayer = CommandCharacter.Get();

	if (CombatInputMode != ECXCombatInputMode::Command ||
		MenuState == ECommandMenuState::TargetSelect ||
		!IsValid(CommandPlayer) ||
		CommandPlayer->GetIsAimMode())
	{
		return;
	}

	CommandPlayer->SkipTurn();
}

void ACXCommandCharacter::HandleInputJustTiming()
{
	if (CombatInputMode != ECXCombatInputMode::JustTiming)
	{
		return;
	}

	ACXCombatCharacterPlayer* Player = CommandCharacter.Get();

	if (!IsValid(Player) || !Player->GetIsCharacterAlive() || !Player->GetIsTurn())
	{
		return;
	}

	if (UCXJustTimingComponent* Timing = Player->FindComponentByClass<UCXJustTimingComponent>())
	{
		Timing->HandleInput();
	}

	/*UE_LOG(LogTemp, Display, TEXT("[저스트타이밍] 스페이스 입력: %s"), *Player->GetName());*/
}

void ACXCommandCharacter::HandleCommandRequest(const FGameplayTag& InChannel, const FCXCombatCommandRequestMessage& InMessage)
{
	switch (InMessage.Request)
	{
	case ECXCombatCommandRequest::SkipTurn:
		HandleInputSkipTurn();
		break;

	case ECXCombatCommandRequest::Flee:
		HandleInputFlee();
		break;

	default:
		break;
	}
}

void ACXCommandCharacter::NotifyCommandAvailability()
{
	const ACXCombatCharacterPlayer* Player = CommandCharacter.Get();

	// 조준 중에는 턴 넘기기 / 도망을 막아 조준 상태로 턴이 끝나지 않게 함
	const bool bAvailable = CombatInputMode == ECXCombatInputMode::Command
		&& MenuState != ECommandMenuState::TargetSelect
		&& IsValid(Player)
		&& !Player->GetIsAimMode();

	// 보스전에서는 도망 불가
	const UCXCombatManagerSubsystem* CombatManager = UCXCombatManagerSubsystem::TryGet(this);
	const bool bInFleeAvailable = bAvailable && CombatManager && CombatManager->CanFlee();

	// 행동 / 스킬 메뉴 전환에서는 bAvailable이 그대로라 아래 중복 검사보다 먼저 호출
	NotifyCommandMenu();

	if (bCommandAvailable == bAvailable && bFleeAvailable == bInFleeAvailable)
	{
		return;
	}

	bCommandAvailable = bAvailable;
	bFleeAvailable = bInFleeAvailable;

	if (UCXGameplayMessageSubsystem::HasInstance(this))
	{
		FCXCombatCommandAvailabilityMessage Message;
		Message.bAvailable = bAvailable;
		Message.bFleeAvailable = bInFleeAvailable;

		UCXGameplayMessageSubsystem::Get(this).BroadcastMessage(CXVIGameplayTags::Message_Combat_CommandAvailabilityChanged, Message);
	}
}

void ACXCommandCharacter::NotifyCommandMenu()
{
	ACXCombatCharacterPlayer* Player = CommandCharacter.Get();
	const bool bMenuActive = CombatInputMode == ECXCombatInputMode::Command && IsValid(Player);
	const bool bAimMode = bMenuActive && Player->GetIsAimMode();

	// 비활성 상태끼리는 메뉴 상태나 캐릭터가 달라도 전달할 변화가 없음
	if (bMenuActive == bCommandMenuActive
		&& (!bMenuActive
			|| (MenuState == LastCommandMenuState
				&& bAimMode == bLastCommandMenuAimMode
				&& Player == LastCommandMenuCharacter.Get())))
	{
		return;
	}

	bCommandMenuActive = bMenuActive;
	bLastCommandMenuAimMode = bAimMode;
	LastCommandMenuState = MenuState;
	LastCommandMenuCharacter = Player;

	if (UCXGameplayMessageSubsystem::HasInstance(this))
	{
		FCXCombatCommandMenuMessage Message;
		Message.bMenuActive = bMenuActive;
		Message.MenuState = MenuState;
		Message.bAimMode = bAimMode;
		Message.CommandCharacter = Player;
		Message.Selection = SavedSelection;

		UCXGameplayMessageSubsystem::Get(this).BroadcastMessage(CXVIGameplayTags::Message_Combat_CommandMenuChanged, Message);
	}
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
	if (Selection.Type == ECXActionType::Skill && (!Player->PrepareSelectedSkill(Selection.SlotIndex) || !Player->CanActivateSelectSkill(Selection.SlotIndex)))
	{
		return;
	}

	

	// SubmitAction 내부의 델리게이트가 동기 실행될 수 있어 전송 전에 메뉴를 준비한다.
	TargetReturnMenu = MenuState;
	Player->SubmitAction(Selection);
}

void ACXCommandCharacter::SelectItemSlot(int32 SlotIndex)
{
	ACXCombatCharacterPlayer* Player = CommandCharacter.Get();
	const FName ItemId = IsValid(Player) ? Player->GetItemIdForSlot(SlotIndex) : NAME_None;

	if (ItemId.IsNone())
	{
		UE_LOG(LogTemp, Display, TEXT("%d번 아이템 슬롯이 비어 있음"), SlotIndex + 1);
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("%d번 아이템 선택 : %s"), SlotIndex + 1, *ItemId.ToString());

	SavedSelection.Clear();
	SavedSelection.Type = ECXActionType::Item;
	SavedSelection.ItemId = ItemId;
	SavedSelection.TargetTeam = ECXTargetTeam::Player;

	SubmitActionToPlayer(SavedSelection);
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

	if (const UCXCombatManagerSubsystem* CombatManager = UCXCombatManagerSubsystem::TryGet(this))
	{
		CombatManager->RequestToggleAimCamera(CommandCharacter.Get());
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

	const TWeakObjectPtr<APawn> InitialTarget = TargetCandidates.IsEmpty()
		? TWeakObjectPtr<APawn>() : TargetCandidates[0];

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
		const FVector ScreenRight = FRotationMatrix(ViewRotation).GetUnitAxis(EAxis::Y);

		// 타겟 카메라로 전환하기 전에 화면의 왼쪽부터 오른쪽으로 정렬한다.
		TargetCandidates.StableSort([ScreenRight](const TWeakObjectPtr<APawn>& Left, const TWeakObjectPtr<APawn>& Right)
		{
			return FVector::DotProduct(Left->GetActorLocation(), ScreenRight)
				< FVector::DotProduct(Right->GetActorLocation(), ScreenRight);
		});
	}

	SelectedTargetIndex = TargetCandidates.IndexOfByKey(InitialTarget);

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

	const int32 PreviousTargetIndex = SelectedTargetIndex;
	for (int32 Attempt = 0; Attempt < Count; ++Attempt)
	{
		SelectedTargetIndex = (SelectedTargetIndex + Direction + Count) % Count;

		ACXCombatCharacterBase* Target = Cast<ACXCombatCharacterBase>(TargetCandidates[SelectedTargetIndex].Get());

		if (IsValid(Target) && Target->GetIsCharacterAlive())
		{
			UE_LOG(LogTemp, Display, TEXT("선택 대상: %s"), *Target->GetName());
			if (SelectedTargetIndex != PreviousTargetIndex)
			{
				RefreshMenuCamera();
			}

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


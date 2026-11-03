// Fill out your copyright notice in the Description page of Project Settings.

#include "CXVI/Foundation/Combat/CXCombatManagerSubsystem.h"

#include "CXVI/Actor/Combat/CXCombatArena.h"
#include "CXVI/Character/Combat/CXCharacterPartyData.h"
#include "CXVI/Character/Combat/CXCombatCharacterBase.h"
#include "CXVI/Character/Combat/CXCombatCharacterPlayer.h"
#include "CXVI/Character/Combat/CXCommandCharacter.h"
#include "CXVI/Foundation/Combat/CXTurnManager.h"
#include "CXVI/Foundation/Combat/CombatMessage/CXCombatMessageData.h"

#include "Character/Combat/CXCombatCharacterEnemy.h"
#include "Foundation/GameplayData/CXGameplayDataSubsystem.h"
#include "Foundation/UserData/CXUserDataSubsystem.h"
#include "Foundation/UserData/CXEnemyDefinition.h"
#include "Foundation/UserData/CXCharacterDefinition.h"

#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"
#include "Foundation/Scene/CXSceneSubsystem.h"
#include "Level/CXLevelTransitionSubsystem.h"
#include "Level/CXLevelDefinition.h"
#include "Tags/CXGameplayTags.h"

#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatManagerSubsystem)

DEFINE_LOG_CATEGORY(LogCXCombat);

namespace CXCombatManager
{
	// TODO : 임시. 파티 정보 출처가 정해지면 제거
	// 요청한 파티가 비어 있으면 테스트 파티 사용 (테스트 파티도 없으면 빈 배열 -> CanStartBattle에서 거절)
	static TArray<TSubclassOf<ACXCombatCharacterBase>> ResolvePartyClasses(const TArray<TSubclassOf<ACXCombatCharacterBase>>& InRequested, const UCXCharacterPartyData* InTestParty)
	{
		if (!InRequested.IsEmpty())
		{
			return InRequested;
		}

		return InTestParty ? InTestParty->CharacterArray : TArray<TSubclassOf<ACXCombatCharacterBase>>();
	}
}

UCXCombatManagerSubsystem& UCXCombatManagerSubsystem::Get(const UObject* WorldContextObject)
{
	UCXCombatManagerSubsystem* Subsystem = TryGet(WorldContextObject);
	check(Subsystem);
	return *Subsystem;
}

UCXCombatManagerSubsystem* UCXCombatManagerSubsystem::TryGet(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	return World ? World->GetSubsystem<UCXCombatManagerSubsystem>() : nullptr;
}

const TArray<TWeakObjectPtr<ACXCombatCharacterBase>> UCXCombatManagerSubsystem::GetUpcomingTurns() const
{
	if (!IsValid(TurnManager))
	{
		return TArray<TWeakObjectPtr<ACXCombatCharacterBase>>();
	}

	return TurnManager->GetUpcomingTurns();
}

void UCXCombatManagerSubsystem::PostInitialize()
{
	Super::PostInitialize();

	TurnManager = NewObject<UCXTurnManager>(this);

	TurnManager->OnBattleEnded.AddUObject(this, &ThisClass::HandleBattleEnded);
	TurnManager->OnPlayerInputEnabledChanged.AddUObject(this, &ThisClass::HandlePlayerInputModeChanged);
	TurnManager->OnUpcomingTurnsChanged.AddUObject(this, &ThisClass::HandleUpcomingTurnsChanged);
}

bool UCXCombatManagerSubsystem::RequestStartBattle(FName InEncounterId, const TArray<TSubclassOf<ACXCombatCharacterBase>>& InPlayerClasses, const TArray<TSubclassOf<ACXCombatCharacterBase>>& InEnemyClasses)
{
	const UCXLevelDefinition* LevelDefinition = GetCurrentLevelDefinition();

	FCXCombatEnterParams Params;
	Params.EncounterId = InEncounterId;
	//Params.PlayerClasses = CXCombatManager::ResolvePartyClasses(InPlayerClasses, LevelDefinition ? LevelDefinition->TestPlayerParty.Get() : nullptr);
	//Params.EnemyClasses = CXCombatManager::ResolvePartyClasses(InEnemyClasses, LevelDefinition ? LevelDefinition->TestEnemyParty.Get() : nullptr);

	// 플레이어 생성
	if (!InPlayerClasses.IsEmpty())
	{
		Params.PlayerClasses.Reserve(InPlayerClasses.Num());
		for (const auto& PlayerClass : InPlayerClasses)
		{
			FCXCombatSpawnEntry& Entry = Params.PlayerClasses.AddDefaulted_GetRef();
			Entry.CharacterClass = PlayerClass;
		}
	}
	else
	{
		const UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this);
		if (!UserData)
		{
			UE_LOG(LogCXCombat, Warning, TEXT("유저 데이터를 읽어올수 없습니다"));
			return false;
		}

		if (!UCXGameplayDataSubsystem::HasInstance(this))
		{
			UE_LOG(LogCXCombat, Warning, TEXT("[아군 스폰] GameplayDataSubsystem 조회 실패"));
			return false;
		}

		const UCXGameplayDataSubsystem& GameplayData = UCXGameplayDataSubsystem::Get(this);

		const auto& PartyMembers = UserData->GetPartyMembers();
		Params.PlayerClasses.Reserve(PartyMembers.Num());

		for (int32 MemberIndex = 0; MemberIndex < PartyMembers.Num(); ++MemberIndex)
		{
			const FCXPartyMemberState& Member = PartyMembers[MemberIndex];

			const UCXCharacterDefinition* Definition = UserData->LoadCharacterDefinition(Member.CharacterId);

			UClass* PlayerClass = Definition ? Definition->CombatCharacterClass.LoadSynchronous() : nullptr;

			if (!PlayerClass || !PlayerClass->IsChildOf(ACXCombatCharacterPlayer::StaticClass()) ||
				PlayerClass->HasAnyClassFlags(CLASS_Abstract))
			{
				UE_LOG(LogCXCombat, Warning, TEXT("[아군 스폰] 최대 HP 조회 실패. ID=%s Level=%d"),
					*Member.CharacterId.ToString(), Member.Level);

				return false;
			}

			const float MaxHealth = UserData->GetMemberMaxHealth(MemberIndex);
			if (!FMath::IsFinite(MaxHealth) || MaxHealth <= 0.f)
			{
				UE_LOG(LogCXCombat, Warning,
					TEXT("[아군 스폰] 최대 HP 조회 실패. ID=%s Level=%d"),
					*Member.CharacterId.ToString(), Member.Level);

				return false;
			}

			FCXCombatSpawnEntry& Entry = Params.PlayerClasses.AddDefaulted_GetRef();
			Entry.CharacterClass = PlayerClass;
			Entry.InitialAttributes.MaxHealth = MaxHealth;
			Entry.InitialAttributes.CurrentHealth = Member.CurrentHealth;
			
			FCXInitialAttributes& Stats = Entry.InitialAttributes;

			Stats.CombatSpeed = GameplayData.EvaluateCharacterStat(Definition->StatRowPrefix, "CombatSpeed", Member.Level);
			Stats.AttackPower = GameplayData.EvaluateCharacterStat(Definition->StatRowPrefix, "AttackPower", Member.Level);
			Stats.Defense = GameplayData.EvaluateCharacterStat(Definition->StatRowPrefix, "Defense", Member.Level);
			Stats.CriticalChance = GameplayData.EvaluateCharacterStat(Definition->StatRowPrefix, "CriticalChance", Member.Level);
			Stats.CriticalDamageMultiplier = GameplayData.EvaluateCharacterStat(Definition->StatRowPrefix, "CriticalDamageMultiplier", Member.Level);
			Entry.bHasInitialAttributes = true;


			UE_LOG(LogCXCombat, Display,
				TEXT("[아군 스폰 준비] 슬롯=%d ID=%s Level=%d HP=%.1f/%.1f"),
				MemberIndex, *Member.CharacterId.ToString(), Member.Level,
				Member.CurrentHealth, MaxHealth);
		}

		/*for (const FCXPartyMemberState& Member : PartyMembers)
		{
			const UCXCharacterDefinition* Definition = UserData->LoadCharacterDefinition(Member.CharacterId);

			UClass* PlayerClass = Definition ? Definition->CombatCharacterClass.LoadSynchronous() : nullptr;

			if (!PlayerClass || !PlayerClass->IsChildOf(ACXCombatCharacterPlayer::StaticClass()) || PlayerClass->HasAnyClassFlags(CLASS_Abstract))
			{
				UE_LOG(LogCXCombat, Warning, TEXT("아군 전투 클래스를 로드할 수 없습니다. ID=%s"), *Member.CharacterId.ToString());

				return false;
			}

			FCXCombatSpawnEntry& Entry = Params.PlayerClasses.AddDefaulted_GetRef();

			Entry.CharacterClass = PlayerClass;

			UE_LOG(LogCXCombat, Display, TEXT("[아군 스폰 준비] ID=%s / 클래스=%s"), *Member.CharacterId.ToString(), *GetNameSafe(PlayerClass));
		}*/

	}

	// 적 생성
	if (!InEnemyClasses.IsEmpty())
	{
		for (const auto& EnemyClass : InEnemyClasses)
		{
			FCXCombatSpawnEntry& Entry = Params.EnemySpawns.AddDefaulted_GetRef();
			Entry.CharacterClass = EnemyClass;
		}
	}
	else
	{
		if (!UCXGameplayDataSubsystem::HasInstance(this))
		{
			return false;
		}
		
		const UCXGameplayDataSubsystem& GameplayData = UCXGameplayDataSubsystem::Get(this);

		const FCXEncounterRow* Encounter = GameplayData.FindEncounterRow(InEncounterId);

		if (!Encounter || Encounter->Enemies.IsEmpty())
		{
			UE_LOG(LogCXCombat, Warning, TEXT("인카운터가 없거나 적 파티가 비어있습니다. ID=%s"), *InEncounterId.ToString());

			return false;
		}

		for (const FPrimaryAssetId& EnemyId : Encounter->Enemies)
		{
			UCXEnemyDefinition* Definition = GameplayData.LoadEnemyDefinition(EnemyId);
			UClass* EnemyClass = Definition
				? Definition->CombatCharacterClass.LoadSynchronous() : nullptr;

			if (!EnemyClass || !EnemyClass->IsChildOf(ACXCombatCharacterEnemy::StaticClass()) || EnemyClass->HasAnyClassFlags(CLASS_Abstract))
			{
				UE_LOG(LogCXCombat, Warning, TEXT("몬스터 정의 또는 전투 클래스 확인 필요, Encounter=%s, Enemy=%s"), *InEncounterId.ToString(), *EnemyId.ToString());

				return false;
			}

			const FCXMonsterStatRow* Row = GameplayData.FindMonsterStatRow(Definition->StatRowPrefix);

			if (!Row || !FMath::IsFinite(Row->MaxHealth) || Row->MaxHealth <= 0.f)
			{
				UE_LOG(LogCXCombat, Warning,
					TEXT("[적 스폰] 스탯 행 확인 필요. Enemy=%s Row=%s"),
					*EnemyId.ToString(), *Definition->StatRowPrefix.ToString());

				return false;
			}

			FCXCombatSpawnEntry& Entry = Params.EnemySpawns.AddDefaulted_GetRef();
			Entry.CharacterClass = EnemyClass;

			FCXInitialAttributes& Stats = Entry.InitialAttributes;
			Stats.MaxHealth = Row->MaxHealth;
			Stats.CurrentHealth = Row->MaxHealth;
			Stats.CombatSpeed = Row->CombatSpeed;
			Stats.AttackPower = Row->AttackPower;
			Stats.Defense = Row->Defense;
			Stats.CriticalChance = Row->CriticalChance;
			Stats.CriticalDamageMultiplier = Row->CriticalDamageMultiplier;
			Stats.DodgeChance = Row->DodgeChance;
			Entry.bHasInitialAttributes = true;

			UE_LOG(LogCXCombat, Display,
				TEXT("[적 스폰 준비] Row=%s HP=%.0f ATK=%.0f SPD=%.0f"),
				*Definition->StatRowPrefix.ToString(),
				Stats.MaxHealth, Stats.AttackPower, Stats.CombatSpeed);
		}

	
	}

	if (!CanStartBattle(Params))
	{
		return false;
	}

	UCXSceneDefinition* CombatScene = LevelDefinition ? LevelDefinition->CombatScene.Get() : nullptr;
	if (nullptr == CombatScene)
	{
		UE_LOG(LogCXCombat, Warning, TEXT("전투 요청 거절 : 현재 레벨 정의에 CombatScene이 없습니다."));
		return false;
	}

	// 전환 설정에 따라 Push 안에서 OnEnter -> BeginBattle이 바로 호출될 수 있으므로 Push 전에 상태 변경
	// Push 이후 OnEnter 전까지 들어오는 재요청도 이 상태로 거절
	CombatState = ECXCombatState::Entering;

	// 전환 중이면 PushScene이 거부함
	if (!UCXSceneSubsystem::Get(this).PushScene(CombatScene, FCXSceneContext::Make(Params)))
	{
		CombatState = ECXCombatState::Idle;
		return false;
	}

	UE_LOG(LogCXCombat, Log, TEXT("전투 진입 요청 : %s"), *InEncounterId.ToString());
	return true;
}

bool UCXCombatManagerSubsystem::BeginBattle(const FCXBattleSetup& InSetup)
{
	// RequestStartBattle을 거치지 않은 진입(디버그 Scene Push 등)은 시작하지 않음
	if (ECXCombatState::Entering != CombatState)
	{
		UE_LOG(LogCXCombat, Warning, TEXT("BeginBattle 무시 : 진입 요청 상태가 아닙니다."));
		return false;
	}

	ACXCommandCharacter* CommandPawn = InSetup.CommandPawn;
	APlayerController* PlayerController = IsValid(CommandPawn) ? Cast<APlayerController>(CommandPawn->GetController()) : nullptr;
	if (!IsValid(TurnManager) || !IsValid(InSetup.Arena) || nullptr == PlayerController)
	{
		UE_LOG(LogCXCombat, Warning, TEXT("BeginBattle 실패 : 아레나 또는 빙의된 커맨드 Pawn이 없습니다."));
		return false;
	}

	if (InSetup.Players.IsEmpty() || InSetup.Enemies.IsEmpty())
	{
		UE_LOG(LogCXCombat, Warning, TEXT("BeginBattle 실패 : 전투원이 없습니다."));
		return false;
	}

	CombatArena = InSetup.Arena;
	WeakCommandCharacter = CommandPawn;

	// 빙의는 전투 Scene(UCXGameScene::OnEnter)이 먼저 처리함 -> 파티 등록만
	for (ACXCombatCharacterBase* Character : InSetup.Players)
	{
		if (ACXCombatCharacterPlayer* Player = Cast<ACXCombatCharacterPlayer>(Character))
		{
			CommandPawn->AddCommandCharacter(Player);
		}
	}

	PlayerController->SetViewTargetWithBlend(InSetup.Arena.Get(), 0.f);

	// StartBattle 안에서 입력 모드 변경 / 종료가 바로 일어날 수 있으므로 먼저 상태 변경
	CombatState = ECXCombatState::InBattle;
	TurnManager->StartBattle(InSetup.Players, InSetup.Enemies);

	return true;
}

void UCXCombatManagerSubsystem::EndBattle()
{
	if (ECXCombatState::Idle == CombatState)
	{
		return;
	}

	// 전투원을 파괴하기 전에 턴 진행과 바인딩 정리 (전투원 정리는 전투 Scene이 아레나에 요청)
	if (IsValid(TurnManager))
	{
		TurnManager->StopBattle();
	}

	CombatArena.Reset();
	WeakCommandCharacter.Reset();

	CombatState = ECXCombatState::Idle;
}

bool UCXCombatManagerSubsystem::CanStartBattle(const FCXCombatEnterParams& InParams) const
{
	if (ECXCombatState::Idle != CombatState)
	{
		UE_LOG(LogCXCombat, Warning, TEXT("전투 요청 거절 : 이미 전투 중입니다."));
		return false;
	}

	if (!IsValid(TurnManager))
	{
		return false;
	}

	// TODO : 인카운터 판단 기준이 정해지면 추가 (현재는 None 여부만 확인)
	if (InParams.EncounterId.IsNone())
	{
		UE_LOG(LogCXCombat, Warning, TEXT("전투 요청 거절 : EncounterId가 없습니다."));
		return false;
	}

	if (InParams.PlayerClasses.IsEmpty() || InParams.EnemySpawns.IsEmpty())
	{
		UE_LOG(LogCXCombat, Warning, TEXT("전투 요청 거절 : 파티 정보가 비어 있습니다. (요청 파티 또는 레벨 정의의 TestPlayerParty / TestEnemyParty 확인)"));
		return false;
	}

	const UCXSceneSubsystem* SceneSubsystem = UCXSceneSubsystem::TryGet(this);
	if (nullptr == SceneSubsystem || SceneSubsystem->IsTransitioning())
	{
		UE_LOG(LogCXCombat, Warning, TEXT("전투 요청 거절 : Scene 전환 중이거나 Scene Subsystem이 없습니다."));
		return false;
	}

	return true;
}

const UCXLevelDefinition* UCXCombatManagerSubsystem::GetCurrentLevelDefinition() const
{
	const UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this);
	return LevelTransition ? LevelTransition->GetCurrentLevelDefinition() : nullptr;
}

void UCXCombatManagerSubsystem::HandleBattleEnded(bool bInVictory, double InElapsedSeconds)
{
	if (ECXCombatState::InBattle != CombatState)
	{
		return;
	}

	LastCombatElapsedSeconds = InElapsedSeconds;

	// 종료 알림만 방송. 필드 복귀와 정리 시점은 받는 쪽(전투 Scene)이 결정
	FCXCombatEndMessage Message;
	Message.bVictory = bInVictory;
	Message.ElapsedSeconds = InElapsedSeconds;

	UCXGameplayMessageSubsystem::Get(this).BroadcastMessage(CXVIGameplayTags::Message_Combat_End, Message);
}

void UCXCombatManagerSubsystem::HandlePlayerInputModeChanged(ECXCombatInputMode NewInputMode)
{
	ACXCommandCharacter* CommandCharacter = WeakCommandCharacter.Get();
	if (!IsValid(CommandCharacter))
	{
		return;
	}

	CommandCharacter->SetCombatInputMode(NewInputMode);
	//CommandCharacter->SetCombatInputMode(bAllowed ? ECombatInputMode::Command : ECombatInputMode::Disabled);
}

void UCXCombatManagerSubsystem::HandleUpcomingTurnsChanged()
{
	UCXGameplayMessageSubsystem::Get(this).BroadcastMessage(CXVIGameplayTags::Message_Combat_TurnOrderChanged, FCXTurnOrderChangedMessage());
}

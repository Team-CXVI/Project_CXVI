// Fill out your copyright notice in the Description page of Project Settings.


#include "Foundation/UserData/CXUserDataSubsystem.h"

#include "Foundation/GameplayData/CXGameplayDataSubsystem.h"
#include "Foundation/GameplayData/CXGameplayDataSettings.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"
#include "Level/CXLevelTransitionSettings.h"
#include "Tags/CXGameplayTags.h"
#include "CXCharacterDefinition.h"
#include "CXNewGameData.h"
#include "CXSaveGame.h"

#include <Kismet/GameplayStatics.h>
#include <Engine/AssetManager.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXUserDataSubsystem)

DEFINE_LOG_CATEGORY(LogCXUserData);

namespace CXUserData
{
	// 최대 HP 커브 행 이름의 스탯 부분 (<StatRowPrefix>.MaxHealth)
	static const FName MaxHealthStatName(TEXT("MaxHealth"));

	// 싱글 플레이 기준 저장 유저 인덱스
	static constexpr int32 SaveUserIndex = 0;
}

UCXUserDataSubsystem& UCXUserDataSubsystem::Get(const UObject* WorldContextObject)
{
	UCXUserDataSubsystem* Subsystem = TryGet(WorldContextObject);
	check(Subsystem);
	return *Subsystem;
}

UCXUserDataSubsystem* UCXUserDataSubsystem::TryGet(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (nullptr == World)
	{
		return nullptr;
	}

	const UGameInstance* GameInstance = World->GetGameInstance();
	if (nullptr == GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UCXUserDataSubsystem>();
}

void UCXUserDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// 변경 메시지 방송, 최대 HP 계산에 필요한 서브시스템을 먼저 초기화
	Collection.InitializeDependency<UCXGameplayMessageSubsystem>();
	Collection.InitializeDependency<UCXGameplayDataSubsystem>();

	Super::Initialize(Collection);

	// 타이틀을 거치지 않고 인게임 레벨을 바로 실행해도(PIE) 데이터가 있도록 새 게임 데이터로 시작
	StartNewGame();
}

void UCXUserDataSubsystem::Deinitialize()
{
	CharacterDefinitions.Reset();

	Super::Deinitialize();
}

bool UCXUserDataSubsystem::StartNewGame()
{
	const UCXGameplayDataSettings* Settings = GetDefault<UCXGameplayDataSettings>();
	const UCXNewGameData* NewGameData = Settings->NewGameData.LoadSynchronous();
	if (nullptr == NewGameData)
	{
		UE_LOG(LogCXUserData, Error, TEXT("StartNewGame 실패: NewGameData가 없습니다. (Project Settings > Game > CXVI Gameplay Data)"));
		return false;
	}

	UserData = FCXUserData();

	for (const FCXNewGameMember& NewMember : NewGameData->PartyMembers)
	{
		FCXPartyMemberState& Member = UserData.Party.AddDefaulted_GetRef();
		Member.CharacterId = NewMember.CharacterId;
		Member.Level = FMath::Max(1, NewMember.StartLevel);
	}

	// 최대 HP 계산에 정의 데이터가 필요하므로 파티 구성 후 로드
	LoadCharacterDefinitions();

	for (FCXPartyMemberState& Member : UserData.Party)
	{
		Member.CurrentHealth = CalculateMaxHealth(Member);
		if (Member.CurrentHealth <= 0.f)
		{
			UE_LOG(LogCXUserData, Error, TEXT("최대 HP를 계산할 수 없습니다: %s Lv.%d (캐릭터 정의 / CT_CharacterStat의 <StatRowPrefix>.MaxHealth 행 확인)"), *Member.CharacterId.ToString(), Member.Level);
		}
	}

	for (const FCXItemStack& Item : NewGameData->StartItems)
	{
		UserData.Inventory.AddCount(Item.ItemId, Item.Count);
	}

	UserData.SaveLocation.LevelId = GetDefault<UCXLevelTransitionSettings>()->NewGameLevel;
	UserData.SaveLocation.EntryTag = NAME_None;

	CurrentSlotIndex = INDEX_NONE;
	PlayTimeBaseSeconds = 0.0;
	SessionStartSeconds = FPlatformTime::Seconds();

	UE_LOG(LogCXUserData, Log, TEXT("New game started. Party: %d, Items: %d"), UserData.Party.Num(), UserData.Inventory.Items.Num());

	BroadcastPartyChanged(INDEX_NONE);
	BroadcastInventoryChanged(NAME_None, 0);
	return true;
}

bool UCXUserDataSubsystem::LoadGame(int32 InSlotIndex)
{
	const FString SlotName = MakeSlotName(InSlotIndex);
	const UCXSaveGame* SaveGameObject = Cast<UCXSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, CXUserData::SaveUserIndex));
	if (nullptr == SaveGameObject)
	{
		UE_LOG(LogCXUserData, Warning, TEXT("LoadGame 실패: 슬롯 %d (%s)를 불러올 수 없습니다."), InSlotIndex, *SlotName);
		return false;
	}

	if (SaveGameObject->Header.SaveVersion > UCXSaveGame::CurrentVersion)
	{
		UE_LOG(LogCXUserData, Error, TEXT("LoadGame 실패: 지원하지 않는 저장 버전입니다. (%d > %d)"), SaveGameObject->Header.SaveVersion, UCXSaveGame::CurrentVersion);
		return false;
	}

	// TODO : 저장 형식이 바뀌면 여기서 이전 버전 변환

	UserData = SaveGameObject->UserData;
	LoadCharacterDefinitions();

	// 자동 저장을 불러온 경우 수동 슬롯은 정해지지 않음 (다음 수동 저장 때 결정)
	CurrentSlotIndex = IsManualSlotIndex(InSlotIndex) ? InSlotIndex : INDEX_NONE;
	PlayTimeBaseSeconds = SaveGameObject->Header.PlayTimeSeconds;
	SessionStartSeconds = FPlatformTime::Seconds();

	UE_LOG(LogCXUserData, Log, TEXT("Game loaded from slot %d."), InSlotIndex);

	BroadcastPartyChanged(INDEX_NONE);
	BroadcastInventoryChanged(NAME_None, 0);
	return true;
}

bool UCXUserDataSubsystem::SaveGame(int32 InSlotIndex)
{
	if (false == IsManualSlotIndex(InSlotIndex))
	{
		UE_LOG(LogCXUserData, Warning, TEXT("SaveGame 무시: %d는 수동 저장 슬롯이 아닙니다. (1 ~ %d)"), InSlotIndex, GetDefault<UCXGameplayDataSettings>()->MaxManualSaveSlots);
		return false;
	}

	return SaveToSlotInternal(InSlotIndex);
}

bool UCXUserDataSubsystem::AutoSave()
{
	return SaveToSlotInternal(AutoSaveSlotIndex);
}

bool UCXUserDataSubsystem::DoesSlotExist(int32 InSlotIndex) const
{
	return UGameplayStatics::DoesSaveGameExist(MakeSlotName(InSlotIndex), CXUserData::SaveUserIndex);
}

bool UCXUserDataSubsystem::GetSlotHeader(int32 InSlotIndex, FCXSaveHeader& OutHeader) const
{
	const UCXSaveGame* SaveGameObject = Cast<UCXSaveGame>(UGameplayStatics::LoadGameFromSlot(MakeSlotName(InSlotIndex), CXUserData::SaveUserIndex));
	if (nullptr == SaveGameObject)
	{
		return false;
	}

	OutHeader = SaveGameObject->Header;
	return true;
}

double UCXUserDataSubsystem::GetPlayTimeSeconds() const
{
	return PlayTimeBaseSeconds + (FPlatformTime::Seconds() - SessionStartSeconds);
}

const UCXCharacterDefinition* UCXUserDataSubsystem::GetCharacterDefinition(const FPrimaryAssetId& InCharacterId) const
{
	const TObjectPtr<UCXCharacterDefinition>* Found = CharacterDefinitions.Find(InCharacterId);
	return Found ? Found->Get() : nullptr;
}

float UCXUserDataSubsystem::GetMemberMaxHealth(int32 InMemberIndex) const
{
	return UserData.Party.IsValidIndex(InMemberIndex) ? CalculateMaxHealth(UserData.Party[InMemberIndex]) : 0.f;
}

void UCXUserDataSubsystem::SetMemberHealth(int32 InMemberIndex, float InHealth)
{
	if (false == UserData.Party.IsValidIndex(InMemberIndex))
	{
		return;
	}

	FCXPartyMemberState& Member = UserData.Party[InMemberIndex];
	Member.CurrentHealth = ClampHealth(Member, InHealth);

	BroadcastPartyChanged(InMemberIndex);
}

void UCXUserDataSubsystem::SetMemberLevel(int32 InMemberIndex, int32 InLevel, int32 InExp)
{
	if (false == UserData.Party.IsValidIndex(InMemberIndex))
	{
		return;
	}

	FCXPartyMemberState& Member = UserData.Party[InMemberIndex];
	Member.Level = FMath::Max(1, InLevel);
	Member.Exp = FMath::Max(0, InExp);

	// 레벨이 내려가서 최대 HP가 줄어든 경우만 맞춤 (레벨 업 회복은 호출하는 쪽에서 SetMemberHealth)
	Member.CurrentHealth = ClampHealth(Member, Member.CurrentHealth);

	BroadcastPartyChanged(InMemberIndex);
}

int32 UCXUserDataSubsystem::GetItemCount(FName InItemId) const
{
	return UserData.Inventory.GetCount(InItemId);
}

void UCXUserDataSubsystem::AddItem(FName InItemId, int32 InCount)
{
	if (InItemId.IsNone() || InCount <= 0)
	{
		return;
	}

	const int32 NewCount = UserData.Inventory.AddCount(InItemId, InCount);
	BroadcastInventoryChanged(InItemId, NewCount);
}

bool UCXUserDataSubsystem::RemoveItem(FName InItemId, int32 InCount)
{
	if (InCount <= 0 || UserData.Inventory.GetCount(InItemId) < InCount)
	{
		return false;
	}

	const int32 NewCount = UserData.Inventory.AddCount(InItemId, -InCount);
	BroadcastInventoryChanged(InItemId, NewCount);
	return true;
}

void UCXUserDataSubsystem::SetSaveLocation(const FPrimaryAssetId& InLevelId, FName InEntryTag)
{
	UserData.SaveLocation.LevelId = InLevelId;
	UserData.SaveLocation.EntryTag = InEntryTag;
}

void UCXUserDataSubsystem::AddDefeatedSpawnId(FName InSpawnId)
{
	if (false == InSpawnId.IsNone())
	{
		UserData.FieldData.DefeatedSpawnIds.Add(InSpawnId);
	}
}

void UCXUserDataSubsystem::RemoveDefeatedSpawnIds(const TArray<FName>& InSpawnIds)
{
	for (const FName& SpawnId : InSpawnIds)
	{
		UserData.FieldData.DefeatedSpawnIds.Remove(SpawnId);
	}
}

bool UCXUserDataSubsystem::SaveToSlotInternal(int32 InSlotIndex)
{
	// 진행 중인 저장이 끝나면 슬롯별로 다시 저장 (다른 슬롯 요청끼리 덮어쓰지 않음)
	if (bSaveInProgress)
	{
		QueuedSaveSlotIndices.Add(InSlotIndex);
		return true;
	}

	UCXSaveGame* SaveGameObject = Cast<UCXSaveGame>(UGameplayStatics::CreateSaveGameObject(UCXSaveGame::StaticClass()));
	if (nullptr == SaveGameObject)
	{
		return false;
	}

	const bool bAutoSave = (AutoSaveSlotIndex == InSlotIndex);

	// 저장할 데이터 확정
	SaveGameObject->Header.SaveVersion = UCXSaveGame::CurrentVersion;
	SaveGameObject->Header.SaveTime = FDateTime::Now();
	SaveGameObject->Header.PlayTimeSeconds = GetPlayTimeSeconds();
	SaveGameObject->Header.bAutoSave = bAutoSave;
	SaveGameObject->Header.LevelId = UserData.SaveLocation.LevelId;
	SaveGameObject->UserData = UserData;

	// 자동 저장은 현재 수동 슬롯을 바꾸지 않음
	if (false == bAutoSave)
	{
		CurrentSlotIndex = InSlotIndex;
	}

	// 직렬화는 여기서 바로, 파일 쓰기만 비동기 (이후 데이터 변경은 이번 저장에 섞이지 않음)
	bSaveInProgress = true;
	UGameplayStatics::AsyncSaveGameToSlot(SaveGameObject, MakeSlotName(InSlotIndex), CXUserData::SaveUserIndex,
		FAsyncSaveGameToSlotDelegate::CreateUObject(this, &ThisClass::HandleAsyncSaveFinished));

	return true;
}

void UCXUserDataSubsystem::HandleAsyncSaveFinished(const FString& InSlotName, const int32 InUserIndex, bool bInSuccess)
{
	bSaveInProgress = false;

	if (bInSuccess)
	{
		UE_LOG(LogCXUserData, Log, TEXT("Saved: %s"), *InSlotName);
	}
	else
	{
		UE_LOG(LogCXUserData, Error, TEXT("저장 실패: %s"), *InSlotName);
	}

	if (QueuedSaveSlotIndices.Num() > 0)
	{
		TSet<int32>::TIterator It = QueuedSaveSlotIndices.CreateIterator();
		const int32 SlotIndex = *It;
		It.RemoveCurrent();

		SaveToSlotInternal(SlotIndex);
	}
}

bool UCXUserDataSubsystem::IsManualSlotIndex(int32 InSlotIndex) const
{
	return InSlotIndex > AutoSaveSlotIndex && InSlotIndex <= GetDefault<UCXGameplayDataSettings>()->MaxManualSaveSlots;
}

FString UCXUserDataSubsystem::MakeSlotName(int32 InSlotIndex)
{
	return FString::Printf(TEXT("CXVI_Slot_%d"), InSlotIndex);
}

void UCXUserDataSubsystem::LoadCharacterDefinitions()
{
	CharacterDefinitions.Reset();

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (nullptr == AssetManager)
	{
		return;
	}

	for (const FCXPartyMemberState& Member : UserData.Party)
	{
		if (CharacterDefinitions.Contains(Member.CharacterId))
		{
			continue;
		}

		const FSoftObjectPath AssetPath = AssetManager->GetPrimaryAssetPath(Member.CharacterId);
		UCXCharacterDefinition* Definition = Cast<UCXCharacterDefinition>(AssetPath.TryLoad());
		if (nullptr == Definition)
		{
			UE_LOG(LogCXUserData, Error, TEXT("캐릭터 정의를 찾을 수 없습니다: %s (Asset Manager 스캔 경로 확인)"), *Member.CharacterId.ToString());
			continue;
		}

		CharacterDefinitions.Add(Member.CharacterId, Definition);
	}
}

float UCXUserDataSubsystem::CalculateMaxHealth(const FCXPartyMemberState& InMember) const
{
	const UCXCharacterDefinition* Definition = GetCharacterDefinition(InMember.CharacterId);
	const UCXGameplayDataSubsystem* GameplayData = GetGameInstance()->GetSubsystem<UCXGameplayDataSubsystem>();
	if (nullptr == Definition || nullptr == GameplayData)
	{
		return 0.f;
	}

	return GameplayData->EvaluateCharacterStat(Definition->StatRowPrefix, CXUserData::MaxHealthStatName, InMember.Level);
}

float UCXUserDataSubsystem::ClampHealth(const FCXPartyMemberState& InMember, float InHealth) const
{
	const float MaxHealth = CalculateMaxHealth(InMember);
	if (MaxHealth <= 0.f)
	{
		UE_LOG(LogCXUserData, Warning, TEXT("최대 HP를 계산할 수 없어 상한 없이 저장합니다: %s Lv.%d (캐릭터 정의 / CT_CharacterStat의 <StatRowPrefix>.MaxHealth 행 확인)"), *InMember.CharacterId.ToString(), InMember.Level);
		return FMath::Max(0.f, InHealth);
	}

	return FMath::Clamp(InHealth, 0.f, MaxHealth);
}

void UCXUserDataSubsystem::BroadcastPartyChanged(int32 InMemberIndex) const
{
	if (UCXGameplayMessageSubsystem* Messages = GetGameInstance()->GetSubsystem<UCXGameplayMessageSubsystem>())
	{
		FCXPartyChangedMessage Message;
		Message.MemberIndex = InMemberIndex;
		Messages->BroadcastMessage(CXVIGameplayTags::Message_UserData_PartyChanged, Message);
	}
}

void UCXUserDataSubsystem::BroadcastInventoryChanged(FName InItemId, int32 InNewCount) const
{
	if (UCXGameplayMessageSubsystem* Messages = GetGameInstance()->GetSubsystem<UCXGameplayMessageSubsystem>())
	{
		FCXInventoryChangedMessage Message;
		Message.ItemId = InItemId;
		Message.NewCount = InNewCount;
		Messages->BroadcastMessage(CXVIGameplayTags::Message_UserData_InventoryChanged, Message);
	}
}
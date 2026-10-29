// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CXUserDataTypes.h"

#include "CXUserDataSubsystem.generated.h"

class UCXCharacterDefinition;

DECLARE_LOG_CATEGORY_EXTERN(LogCXUserData, Log, All);

/**
 * 유저 데이터 보관 / 수정 / 저장 (파티, 인벤토리, 저장 위치, 필드 처치 기록)
 * - 데이터를 읽고 고치는 함수만 제공. 언제, 왜 고치는지(전투 결과, 휴식, 아이템 효과)는 호출하는 쪽이 정함
 * - 저장 슬롯 : 0 = 자동 저장 전용, 1 ~ MaxManualSaveSlots = 수동 저장
 * - 저장은 호출 시점의 UserData 스냅샷. 저장 시점 판단(전투 중 금지 등)은 호출하는 쪽 책임
 * - 레벨만 저장하고 스탯은 저장하지 않음 (정의 데이터 + 커브로 계산)
 * - 변경 시 Message.UserData.PartyChanged / InventoryChanged 방송 (UI 갱신용)
 */
UCLASS()
class CXVI_API UCXUserDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UCXUserDataSubsystem& Get(const UObject* WorldContextObject);

	static UCXUserDataSubsystem* TryGet(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	// Session
public:
	// 새 게임 데이터(Settings.NewGameData)로 UserData를 초기화
	// 저장 슬롯은 정하지 않음 (저장할 때 결정)
	bool StartNewGame();

	// 슬롯에서 불러오기. 레벨 이동은 호출하는 쪽에서 GetSaveLocation()으로 처리
	bool LoadGame(int32 InSlotIndex);

	// Save
public:
	// 수동 저장 (1 ~ MaxManualSaveSlots)
	bool SaveGame(int32 InSlotIndex);

	// 자동 저장 (슬롯 0)
	bool AutoSave();

	bool DoesSlotExist(int32 InSlotIndex) const;

	// 슬롯 목록 UI용
	bool GetSlotHeader(int32 InSlotIndex, FCXSaveHeader& OutHeader) const;

	// 마지막으로 쓴 수동 슬롯 (없으면 INDEX_NONE)
	int32 GetCurrentSlotIndex() const { return CurrentSlotIndex; }

	double GetPlayTimeSeconds() const;

	// Party
public:
	const TArray<FCXPartyMemberState>& GetPartyMembers() const { return UserData.Party; }

	const UCXCharacterDefinition* GetCharacterDefinition(const FPrimaryAssetId& InCharacterId) const;

	float GetMemberMaxHealth(int32 InMemberIndex) const;

	// HP는 0 ~ 최대 HP로 맞춰서 저장
	void SetMemberHealth(int32 InMemberIndex, float InHealth);

	// 레벨 / 경험치 변경. 현재 HP가 새 최대 HP보다 크면 최대 HP로 맞춤
	void SetMemberLevel(int32 InMemberIndex, int32 InLevel, int32 InExp);

	// Inventory
public:
	const FCXInventory& GetInventory() const { return UserData.Inventory; }

	int32 GetItemCount(FName InItemId) const;

	void AddItem(FName InItemId, int32 InCount);

	// 개수가 부족하면 빼지 않고 false
	bool RemoveItem(FName InItemId, int32 InCount);

	// Save Location
public:
	const FCXSaveLocation& GetSaveLocation() const { return UserData.SaveLocation; }

	void SetSaveLocation(const FPrimaryAssetId& InLevelId, FName InEntryTag);

	// Field
public:
	const TSet<FName>& GetDefeatedSpawnIds() const { return UserData.FieldData.DefeatedSpawnIds; }

	void AddDefeatedSpawnId(FName InSpawnId);

	void RemoveDefeatedSpawnIds(const TArray<FName>& InSpawnIds);

private:
	// Save
	bool SaveToSlotInternal(int32 InSlotIndex);

	void HandleAsyncSaveFinished(const FString& InSlotName, const int32 InUserIndex, bool bInSuccess);

	bool IsManualSlotIndex(int32 InSlotIndex) const;

	static FString MakeSlotName(int32 InSlotIndex);

	// Party
	void LoadCharacterDefinitions();

	float CalculateMaxHealth(const FCXPartyMemberState& InMember) const;

	// HP를 0 ~ 최대 HP로 맞춤. 최대 HP를 계산할 수 없으면(데이터 누락) 상한 없이 반환 + 경고
	float ClampHealth(const FCXPartyMemberState& InMember, float InHealth) const;

	// 변경 사항이 생겼을 때 Message 브로드캐스트
	void BroadcastPartyChanged(int32 InMemberIndex) const;

	void BroadcastInventoryChanged(FName InItemId, int32 InNewCount) const;

public:
	// 자동 저장 슬롯 인덱스
	static constexpr int32 AutoSaveSlotIndex = 0;

private:
	UPROPERTY()
	FCXUserData UserData;

	// 현재 파티의 캐릭터 정의 캐시
	UPROPERTY()
	TMap<FPrimaryAssetId, TObjectPtr<UCXCharacterDefinition>> CharacterDefinitions;

	int32 CurrentSlotIndex = INDEX_NONE;

	// 플레이 시간 = 불러온 값 + 시작 후 경과 시간
	double PlayTimeBaseSeconds = 0.0;

	double SessionStartSeconds = 0.0;

	bool bSaveInProgress = false;

	// 비동기 저장 진행 중에 들어온 저장 요청 (슬롯별 1개. 실제 저장 시점의 최신 데이터로 저장)
	TSet<int32> QueuedSaveSlotIndices;
};

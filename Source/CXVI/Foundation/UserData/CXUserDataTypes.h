// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/PrimaryAssetId.h"

#include "CXUserDataTypes.generated.h"

/**
 * 유저 데이터(세이브 대상) 타입과 유저 데이터 관련 메시지
 * - 저장에는 ID(PrimaryAssetId, 아이템 Row 이름)와 변하는 값만 둠
 * - 레벨로 계산되는 스탯(최대 HP 등)은 저장하지 않고 정의 데이터 + 커브로 계산
 */

// 각 파티원 정보
USTRUCT(BlueprintType)
struct CXVI_API FCXPartyMemberState
{
	GENERATED_BODY()

public:
	bool IsIncapacitated() const { return CurrentHealth <= 0.f; }

public:
	// 캐릭터 정의 ID (UCXCharacterDefinition의 Asset Id)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UserData", meta = (AllowedTypes = "CXCharacterDefinition"))
	FPrimaryAssetId CharacterId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UserData")
	int32 Level = 1;

	// 각 레벨 구간에서 획득한 경험치(레벨이 오르면 0으로 리셋)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UserData")
	int32 Exp = 0;

	// 전투 사이에 이어지는 HP. 0이면 전투 불능
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UserData")
	float CurrentHealth = 0.f;
};

// 각 아이템 별 보유 수량 정보
USTRUCT(BlueprintType)
struct CXVI_API FCXItemStack
{
	GENERATED_BODY()

public:
	// DT_Item의 Row 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UserData")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UserData", meta = (ClampMin = 0))
	int32 Count = 0;
};

// 인벤토리 구조체
USTRUCT(BlueprintType)
struct CXVI_API FCXInventory
{
	GENERATED_BODY()

public:
	int32 GetCount(FName InItemId) const
	{
		const FCXItemStack* Stack = Items.FindByPredicate([InItemId](const FCXItemStack& InStack) { return InStack.ItemId == InItemId; });
		return Stack ? Stack->Count : 0;
	}

	// 개수 변경 -> 뺄 때는 음수로 인자 설정.
	// 0 이하가 되면 항목 제거. 
	// return : 변경 후 개수 반환
	int32 AddCount(FName InItemId, int32 InDelta)
	{
		const int32 Index = Items.IndexOfByPredicate([InItemId](const FCXItemStack& InStack) { return InStack.ItemId == InItemId; });
		const int32 NewCount = FMath::Max(0, (Items.IsValidIndex(Index) ? Items[Index].Count : 0) + InDelta);

		if (0 == NewCount)
		{
			if (Items.IsValidIndex(Index))
			{
				Items.RemoveAt(Index);
			}
			return 0;
		}

		if (Items.IsValidIndex(Index))
		{
			Items[Index].Count = NewCount;
		}
		else
		{
			FCXItemStack& Stack = Items.AddDefaulted_GetRef();
			Stack.ItemId = InItemId;
			Stack.Count = NewCount;
		}
		return NewCount;
	}

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UserData")
	TArray<FCXItemStack> Items;
};

// 이어하기 시작 위치(마지막 저장한 PlayerStart)
USTRUCT(BlueprintType)
struct CXVI_API FCXSaveLocation
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UserData", meta = (AllowedTypes = "CXLevelDefinition"))
	FPrimaryAssetId LevelId;

	// 레벨의 PlayerStart 태그. None이면 레벨 기본 위치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UserData")
	FName EntryTag;
};

// 필드 진행 상태 (자동 저장은 깃발 밖에서 일어나므로 처치 상태 유지 필요)
USTRUCT(BlueprintType)
struct CXVI_API FCXFieldSaveData
{
	GENERATED_BODY()

public:
	// 처치된 적의 SpawnId (휴식으로 리젠되는 적은 휴식 시 제거됨)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UserData")
	TSet<FName> DefeatedSpawnIds;
};

// 세이브 대상 전체
USTRUCT(BlueprintType)
struct CXVI_API FCXUserData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UserData")
	TArray<FCXPartyMemberState> Party;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UserData")
	FCXInventory Inventory;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UserData")
	FCXSaveLocation SaveLocation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UserData")
	FCXFieldSaveData FieldData;
};

// 저장 헤더(각 저장 정보의 대표 데이터)
USTRUCT(BlueprintType)
struct CXVI_API FCXSaveHeader
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "UserData")
	int32 SaveVersion = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UserData")
	FDateTime SaveTime;

	UPROPERTY(BlueprintReadOnly, Category = "UserData")
	double PlayTimeSeconds = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "UserData")
	bool bAutoSave = false;

	UPROPERTY(BlueprintReadOnly, Category = "UserData")
	FPrimaryAssetId LevelId;
};

#pragma region Message

// Message.UserData.PartyChanged 태그로 전달하는 메시지 구조체
// 해당 인덱스의 파티원에 변경사항 발생
USTRUCT(BlueprintType)
struct CXVI_API FCXPartyChangedMessage
{
	GENERATED_BODY()

public:
	// 바뀐 파티원 인덱스. INDEX_NONE이면 파티 전체 (새 게임, 로드)
	UPROPERTY(BlueprintReadOnly, Category = "UserData")
	int32 MemberIndex = INDEX_NONE;
};

// Message.UserData.InventoryChanged 태그로 전달하는 메시지 구조체
// 해당 아이템에 변경사항 발생
USTRUCT(BlueprintType)
struct CXVI_API FCXInventoryChangedMessage
{
	GENERATED_BODY()

public:
	// 바뀐 아이템. None이면 인벤토리 전체 (새 게임, 로드)
	UPROPERTY(BlueprintReadOnly, Category = "UserData")
	FName ItemId;

	UPROPERTY(BlueprintReadOnly, Category = "UserData")
	int32 NewCount = 0;
};

#pragma endregion // Message

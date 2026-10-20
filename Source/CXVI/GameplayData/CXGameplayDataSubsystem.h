// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "Types/CXItemTypes.h"

#include "CXGameplayDataSubsystem.generated.h"

class UCurveTable;

DECLARE_LOG_CATEGORY_EXTERN(LogGameplayData, Log, All)

/**
 * 게임플레이 정적 데이터(DataTable / CurveTable) 로드 및 조회
 * - 테이블 경로는 UCXGameplayDataSettings(ini)에서 가져옴
 * - 읽기 전용. 런타임 상태(인벤토리, 현재 레벨 등)는 두지 않음
 */
UCLASS()
class CXVI_API UCXGameplayDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	static UCXGameplayDataSubsystem& Get(const UObject* WorldContextObject);

	static bool HasInstance(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

public:
	template <typename TRow>
	const TRow* FindRow(const UDataTable* InTable, FName InRowName) const
	{
		static_assert(TIsDerivedFrom<TRow, FTableRowBase>::Value, "TRow must derive from FTableRowBase");

		if (nullptr == InTable)
		{
			return nullptr;
		}

		return InTable->FindRow<TRow>(InRowName, TEXT("CXGameplayDataSubsystem"), /*bWarnIfRowMissing*/false);
	}

	float EvaluateCurve(const UCurveTable* InTable, FName InRowName, float InX, float InDefaultValue = 0.f) const;

	const FCXItemRow* FindItemRow(FName InItemId) const;

	void GetAllItemIds(TArray<FName>& OutItemIds) const;

public:
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "GameplayData|Item", meta = (DisplayName = "Find Item Row", ExpandBoolAsExecs = "ReturnValue"))
	bool K2_FindItemRow(FName InItemId, FCXItemRow& OutRow) const;

private:
	UDataTable* LoadDataTable(const TSoftObjectPtr<UDataTable>& InPath, const UScriptStruct* InExpectedRowStruct, const TCHAR* InDebugName) const;

	UCurveTable* LoadCurveTable(const TSoftObjectPtr<UCurveTable>& InPath, const TCHAR* InDebugName) const;

	static FName MakeStatRowName(FName InOwnerId, FName InStatName);

private:
	UPROPERTY()
	TObjectPtr<UDataTable> ItemTable;

	UPROPERTY()
	TObjectPtr<UCurveTable> CharacterStatCurve;
	
	UPROPERTY()
	TObjectPtr<UCurveTable> MonsterStatCurve;
};

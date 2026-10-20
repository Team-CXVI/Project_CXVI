// Fill out your copyright notice in the Description page of Project Settings.


#include "CXGameplayDataSubsystem.h"
#include "CXGameplayDataSettings.h"
#include "Engine/CurveTable.h"
#include "Engine/GameInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGameplayDataSubsystem)

DEFINE_LOG_CATEGORY(LogGameplayData)

UCXGameplayDataSubsystem& UCXGameplayDataSubsystem::Get(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	check(World);

	UCXGameplayDataSubsystem* Subsystem = UGameInstance::GetSubsystem<UCXGameplayDataSubsystem>(World->GetGameInstance());
	check(Subsystem);

	return *Subsystem;
}

bool UCXGameplayDataSubsystem::HasInstance(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	return World && nullptr != UGameInstance::GetSubsystem<UCXGameplayDataSubsystem>(World->GetGameInstance());
}

void UCXGameplayDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// DeveloperSettings에서 DataTable, CurveData를 찾아서 로드
	const UCXGameplayDataSettings* Settings = GetDefault<UCXGameplayDataSettings>();

	ItemTable = LoadDataTable(Settings->ItemTable, FCXItemRow::StaticStruct(), TEXT("ItemTable"));
	CharacterStatCurve = LoadCurveTable(Settings->CharacterStatCurve, TEXT("CharacterStatCurve"));
	MonsterStatCurve = LoadCurveTable(Settings->MonsterStatCurve, TEXT("MonsterStatCurve"));

	UE_LOG(LogGameplayData, Log, TEXT("Gameplay data loaded."));
}

void UCXGameplayDataSubsystem::Deinitialize()
{
	ItemTable = nullptr;
	CharacterStatCurve = nullptr;
	MonsterStatCurve = nullptr;

	Super::Deinitialize();
}

float UCXGameplayDataSubsystem::EvaluateCurve(const UCurveTable* InTable, FName InRowName, float InX, float InDefaultValue) const
{
	if (nullptr == InTable)
	{
		return InDefaultValue;
	}

	if (const FRealCurve* Curve = InTable->FindCurve(InRowName, TEXT("CXGameplayDataSubsystem"), /*bWarnIfNotFound*/false))
	{
		return Curve->Eval(InX);
	}

	UE_LOG(LogGameplayData, Warning, TEXT("[%s] Row '%s'를 찾을 수 없습니다."), *InTable->GetName(), *InRowName.ToString());
	return InDefaultValue;
}

const FCXItemRow* UCXGameplayDataSubsystem::FindItemRow(FName InItemId) const
{
	return FindRow<FCXItemRow>(ItemTable, InItemId);
}

void UCXGameplayDataSubsystem::GetAllItemIds(TArray<FName>& OutItemIds) const
{
	OutItemIds.Reset();
	if (nullptr != ItemTable)
	{
		OutItemIds = ItemTable->GetRowNames();
	}
}

bool UCXGameplayDataSubsystem::K2_FindItemRow(FName InItemId, FCXItemRow& OutRow) const
{
	if (const FCXItemRow* Row = FindItemRow(InItemId))
	{
		OutRow = *Row;
		return true;
	}

	return false;
}

UDataTable* UCXGameplayDataSubsystem::LoadDataTable(const TSoftObjectPtr<UDataTable>& InPath, const UScriptStruct* InExpectedRowStruct, const TCHAR* InDebugName) const
{
	if (InPath.IsNull())
	{
		UE_LOG(LogGameplayData, Warning, TEXT("[%s] 경로가 설정되지 않았습니다. (Project Settings > Game > CXVI Gameplay Data)"), InDebugName);
		return nullptr;
	}

	UDataTable* Table = InPath.LoadSynchronous();
	if (nullptr == Table)
	{
		UE_LOG(LogGameplayData, Error, TEXT("[%s] 로드 실패: %s"), InDebugName, *InPath.ToString());
		return nullptr;
	}

	// ini 파일을 직접 수정해서 다른 구조의 Table이 들어온 경우를 방지
	const UScriptStruct* RowStruct = Table->GetRowStruct();
	if (InExpectedRowStruct && (nullptr == RowStruct || !RowStruct->IsChildOf(InExpectedRowStruct)))
	{
		UE_LOG(LogGameplayData, Error, TEXT("[%s] Row 구조가 다릅니다. 예상: %s, 실제: %s"),
			InDebugName, *InExpectedRowStruct->GetName(), RowStruct ? *RowStruct->GetName() : TEXT("None"));
		return nullptr;
	}

	return Table;
}

UCurveTable* UCXGameplayDataSubsystem::LoadCurveTable(const TSoftObjectPtr<UCurveTable>& InPath, const TCHAR* InDebugName) const
{
	if (InPath.IsNull())
	{
		UE_LOG(LogGameplayData, Warning, TEXT("[%s] 경로가 설정되지 않았습니다. (Project Settings > Game > CXVI Gameplay Data)"), InDebugName);
		return nullptr;
	}

	UCurveTable* Table = InPath.LoadSynchronous();
	if (nullptr == Table)
	{
		UE_LOG(LogGameplayData, Error, TEXT("[%s] 로드 실패: %s"), InDebugName, *InPath.ToString());
	}

	return Table;
}

FName UCXGameplayDataSubsystem::MakeStatRowName(FName InOwnerId, FName InStatName)
{
	return FName(*FString::Printf(TEXT("%s.%s"), *InOwnerId.ToString(), *InStatName.ToString()));
}

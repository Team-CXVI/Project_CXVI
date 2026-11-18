// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXTurnOrderWidget.h"

#include "Character/Combat/CXCombatCharacterBase.h"
#include "UI/Combat/CXTurnOrderEntryWidget.h"

// TODO : CXLOG Category 정리 후 대체
#include "UI/CXUIManagerSubsystem.h"

#include <Components/CanvasPanel.h>
#include <Components/CanvasPanelSlot.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXTurnOrderWidget)

void UCXTurnOrderWidget::SetTurnOrder(const TArray<FCXTurnOrderEntryData>& InEntries)
{
	const bool bIsFirstFill = ActiveEntries.IsEmpty();

	const TArray<int32> RetainedMap = BuildRetainedMap(InEntries);

	TArray<TObjectPtr<UCXTurnOrderEntryWidget>> NewEntries;
	NewEntries.SetNumZeroed(InEntries.Num());

	for (int32 OldIndex = 0; OldIndex < ActiveEntries.Num(); ++OldIndex)
	{
		UCXTurnOrderEntryWidget* Entry = ActiveEntries[OldIndex];

		if (INDEX_NONE != RetainedMap[OldIndex])
		{
			NewEntries[RetainedMap[OldIndex]] = Entry;
			continue;
		}

		// PlayExit 안에서 바로 완료될 수 있으므로 먼저 등록
		ExitingEntries.Emplace(Entry);
		Entry->PlayExit();
	}

	for (int32 Index = 0; Index < NewEntries.Num(); ++Index)
	{
		UCXTurnOrderEntryWidget* Entry = NewEntries[Index];

		if (nullptr == Entry)
		{
			Entry = CreateEntry(InEntries[Index]);
			if (nullptr == Entry)
			{
				continue;
			}

			NewEntries[Index] = Entry;
			Entry->SnapTo(GetRowPosition(Index));
			Entry->PlayEnter(bIsFirstFill ? EnterStaggerDelay * Index : 0.f);
		}
		else
		{
			Entry->MoveTo(GetRowPosition(Index), MoveDuration);
		}

		Entry->SetIsCurrent(0 == Index);
	}

	NewEntries.RemoveAll([](const TObjectPtr<UCXTurnOrderEntryWidget>& InEntry)
		{
			return nullptr == InEntry;
		});

	ActiveEntries = MoveTemp(NewEntries);
	bHasMovingEntries = true;
}

void UCXTurnOrderWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bHasMovingEntries)
	{
		return;
	}

	bool bStillMoving = false;

	for (UCXTurnOrderEntryWidget* Entry : ActiveEntries)
	{
		bStillMoving |= Entry->TickMove(InDeltaTime);
	}

	// 이동 중에 빠진 칸도 멈추지 않고 이어서 이동
	for (UCXTurnOrderEntryWidget* Entry : ExitingEntries)
	{
		bStillMoving |= Entry->TickMove(InDeltaTime);
	}

	bHasMovingEntries = bStillMoving;
}

TArray<int32> UCXTurnOrderWidget::BuildRetainedMap(const TArray<FCXTurnOrderEntryData>& InEntries) const
{
	const int32 OldNum = ActiveEntries.Num();
	const int32 NewNum = InEntries.Num();

	TArray<int32> RetainedMap;
	RetainedMap.Init(INDEX_NONE, OldNum);

	const auto IsSameCharacter = [this, &InEntries](int32 InOld, int32 InNew)
		{
			const ACXCombatCharacterBase* OldCharacter = ActiveEntries[InOld]->GetCharacter();
			return nullptr != OldCharacter && OldCharacter == InEntries[InNew].Character.Get();
		};

	// Table[O][N] = 이전 목록 O번부터, 새 목록 N번부터 비교했을 때 유지할 수 있는 최대 칸 수
	const int32 Width = NewNum + 1;
	TArray<int32> Table;
	Table.Init(0, (OldNum + 1) * Width);

	for (int32 Old = OldNum - 1; Old >= 0; --Old)
	{
		for (int32 New = NewNum - 1; New >= 0; --New)
		{
			Table[Old * Width + New] = IsSameCharacter(Old, New)
				? Table[(Old + 1) * Width + New + 1] + 1
				: FMath::Max(Table[(Old + 1) * Width + New], Table[Old * Width + New + 1]);
		}
	}

	int32 Old = 0;
	int32 New = 0;
	while (Old < OldNum && New < NewNum)
	{
		if (IsSameCharacter(Old, New))
		{
			RetainedMap[Old] = New;
			++Old;
			++New;
		}
		else if (Table[(Old + 1) * Width + New] >= Table[Old * Width + New + 1])
		{
			++Old;
		}
		else
		{
			++New;
		}
	}

	return RetainedMap;
}

UCXTurnOrderEntryWidget* UCXTurnOrderWidget::CreateEntry(const FCXTurnOrderEntryData& InData)
{
	if (nullptr == EntryClass || nullptr == EntryCanvas)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] EntryClass 또는 EntryCanvas가 없어 턴 순서를 표시하지 않습니다."), *GetName());
		return nullptr;
	}

	UCXTurnOrderEntryWidget* Entry = CreateWidget<UCXTurnOrderEntryWidget>(this, EntryClass);
	if (nullptr == Entry)
	{
		return nullptr;
	}

	Entry->OnExitFinished.BindUObject(this, &ThisClass::HandleEntryExited);
	Entry->SetEntryData(InData);

	// 위치는 RenderTranslation으로 잡으므로 슬롯은 원점에 고정
	if (UCanvasPanelSlot* CanvasSlot = EntryCanvas->AddChildToCanvas(Entry))
	{
		CanvasSlot->SetAutoSize(true);
	}

	return Entry;
}

void UCXTurnOrderWidget::HandleEntryExited(UCXTurnOrderEntryWidget* InEntry)
{
	ExitingEntries.Remove(InEntry);
	InEntry->RemoveFromParent();
}

FVector2D UCXTurnOrderWidget::GetRowPosition(int32 InIndex) const
{
	return FirstRowPosition + RowStep * InIndex;
}

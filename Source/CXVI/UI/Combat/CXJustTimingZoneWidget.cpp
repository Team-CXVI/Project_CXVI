// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXJustTimingZoneWidget.h"

#include "Foundation/Combat/CombatMessage/CXCombatMessageData.h"
#include "Tags/CXGameplayTags.h"
#include "UI/CXUIManagerSubsystem.h"
#include "UI/Combat/CXJustTimingGaugeWidget.h"

#include <Components/Overlay.h>
#include <Components/OverlaySlot.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXJustTimingZoneWidget)

void UCXJustTimingZoneWidget::NativeConstruct()
{
	Super::NativeConstruct();

	JustTimingListener = UCXGameplayMessageSubsystem::Get(this).RegisterListener(CXVIGameplayTags::Message_Combat_JustTiming, this, &ThisClass::HandleJustTiming);
}

void UCXJustTimingZoneWidget::NativeDestruct()
{
	JustTimingListener.Unregister();

	ClearGauges();

	Super::NativeDestruct();
}

void UCXJustTimingZoneWidget::HandleJustTiming(const FGameplayTag& InChannel, const FCXJustTimingMessage& InMessage)
{
	switch (InMessage.Event)
	{
	case ECXJustTimingEvent::Opened:
		AddGauge(InMessage);
		break;

	case ECXJustTimingEvent::Succeeded:
	case ECXJustTimingEvent::Failed:
		if (UCXJustTimingGaugeWidget* Gauge = TakeGauge(InMessage))
		{
			Gauge->ShowResult(InMessage.Event == ECXJustTimingEvent::Succeeded, InMessage.Progress);
		}
		break;

	case ECXJustTimingEvent::Cleared:
		if (UCXJustTimingGaugeWidget* Gauge = TakeGauge(InMessage))
		{
			Gauge->RemoveFromParent();
		}
		break;

	default:
		break;
	}
}

void UCXJustTimingZoneWidget::AddGauge(const FCXJustTimingMessage& InMessage)
{
	if (!IsValid(GaugeOverlay) || !GaugeWidgetClass)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[JustTiming] 표시 실패. Overlay=%d, GaugeWidgetClass=%d (%s 확인)"),
			IsValid(GaugeOverlay), GaugeWidgetClass != nullptr, *GetNameSafe(GetClass()));
		return;
	}

	// 같은 구간이 다시 열리면 이전 게이지는 제거
	if (UCXJustTimingGaugeWidget* OldGauge = TakeGauge(InMessage))
	{
		OldGauge->RemoveFromParent();
	}

	UCXJustTimingGaugeWidget* Gauge = CreateWidget<UCXJustTimingGaugeWidget>(this, GaugeWidgetClass);
	if (!IsValid(Gauge))
	{
		return;
	}

	// 표시 전용. 마우스 입력을 막지 않음
	Gauge->SetVisibility(ESlateVisibility::HitTestInvisible);

	// 마지막 자식 = 가장 위에 그려짐
	if (UOverlaySlot* GaugeSlot = GaugeOverlay->AddChildToOverlay(Gauge))
	{
		GaugeSlot->SetHorizontalAlignment(HAlign_Center);
		GaugeSlot->SetVerticalAlignment(VAlign_Center);
	}

	// 랜덤 위치는 게이지 루트에. 결과 애니메이션(자식 트랙)과 Render Transform이 겹치지 않음
	Gauge->SetRenderTranslation(FVector2D(
		FMath::FRandRange(-RandomOffsetRange.X, RandomOffsetRange.X),
		FMath::FRandRange(-RandomOffsetRange.Y, RandomOffsetRange.Y)));

	const FIntPoint TimingKey(InMessage.HitNumber, InMessage.TimingIndex);

	Gauge->BeginGauge(InMessage.Source.Get(), TimingKey, InMessage.SuccessStartRatio, InMessage.SuccessEndRatio);

	FCXJustTimingGaugeEntry& Entry = ActiveGauges.AddDefaulted_GetRef();
	Entry.Source = InMessage.Source;
	Entry.TimingKey = TimingKey;
	Entry.Gauge = Gauge;
}

UCXJustTimingGaugeWidget* UCXJustTimingZoneWidget::TakeGauge(const FCXJustTimingMessage& InMessage)
{
	const FIntPoint TimingKey(InMessage.HitNumber, InMessage.TimingIndex);

	for (int32 Index = ActiveGauges.Num() - 1; Index >= 0; --Index)
	{
		const FCXJustTimingGaugeEntry& Entry = ActiveGauges[Index];

		// 이미 사라진 게이지는 정리
		if (!Entry.Gauge.IsValid())
		{
			ActiveGauges.RemoveAtSwap(Index);
			continue;
		}

		if (Entry.Source == InMessage.Source && Entry.TimingKey == TimingKey)
		{
			UCXJustTimingGaugeWidget* Gauge = Entry.Gauge.Get();
			ActiveGauges.RemoveAtSwap(Index);
			return Gauge;
		}
	}

	return nullptr;
}

void UCXJustTimingZoneWidget::ClearGauges()
{
	if (IsValid(GaugeOverlay))
	{
		GaugeOverlay->ClearChildren();
	}

	ActiveGauges.Reset();
}
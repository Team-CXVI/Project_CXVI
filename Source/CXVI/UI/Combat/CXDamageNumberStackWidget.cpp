// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXDamageNumberStackWidget.h"

#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "UI/Combat/CXDamageNumberWidget.h"
#include "UI/CXUIManagerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXDamageNumberStackWidget)

void UCXDamageNumberStackWidget::AddDamageResult(ECXDamageResult InResult, float InAmount)
{
	if (!IsValid(NumberOverlay) || !NumberWidgetClass)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[DamageNumber] 표시 실패. Overlay=%d, NumberWidgetClass=%d (%s 확인)"),
			IsValid(NumberOverlay), NumberWidgetClass != nullptr, *GetNameSafe(GetClass()));
		return;
	}

	UCXDamageNumberWidget* NumberWidget = CreateWidget<UCXDamageNumberWidget>(this, NumberWidgetClass);
	if (!IsValid(NumberWidget))
	{
		return;
	}

	// 마지막 자식 = 가장 위에 그려짐
	if (UOverlaySlot* NumberSlot = NumberOverlay->AddChildToOverlay(NumberWidget))
	{
		NumberSlot->SetHorizontalAlignment(HAlign_Center);
		NumberSlot->SetVerticalAlignment(VAlign_Center);
	}

	// 랜덤 위치는 숫자 위젯 루트에. 애니메이션(DamageText)과 Render Transform이 겹치지 않음
	NumberWidget->SetRenderTranslation(FVector2D(
		FMath::FRandRange(-RandomOffsetRange.X, RandomOffsetRange.X),
		FMath::FRandRange(-RandomOffsetRange.Y, RandomOffsetRange.Y)));

	// 위젯 트리에 들어간 뒤 재생. 표시할 내용이 없으면 바로 제거
	if (!NumberWidget->ShowResult(InResult, InAmount))
	{
		NumberWidget->RemoveFromParent();
	}
}
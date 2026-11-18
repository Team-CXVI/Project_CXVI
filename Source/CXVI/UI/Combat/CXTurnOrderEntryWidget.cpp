// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXTurnOrderEntryWidget.h"

#include "Character/Combat/CXCombatCharacterBase.h"

#include <Components/Image.h>
#include <Engine/Texture2D.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXTurnOrderEntryWidget)

void UCXTurnOrderEntryWidget::SetEntryData(const FCXTurnOrderEntryData& InData)
{
	Character = InData.Character;

	if (nullptr == PortraitImage)
	{
		return;
	}

	// 빈 Image는 흰 사각형으로 보이므로 숨김
	if (InData.Portrait.IsNull())
	{
		PortraitImage->SetBrushResourceObject(nullptr);
		PortraitImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	PortraitImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	PortraitImage->SetBrushFromSoftTexture(InData.Portrait);
}

void UCXTurnOrderEntryWidget::SetIsCurrent(bool bInIsCurrent)
{
	if (bIsCurrent == bInIsCurrent)
	{
		return;
	}

	bIsCurrent = bInIsCurrent;

	if (nullptr == CurrentAnim)
	{
		return;
	}

	if (bIsCurrent)
	{
		PlayAnimation(CurrentAnim);
	}
	else
	{
		PlayAnimationReverse(CurrentAnim);
	}
}

void UCXTurnOrderEntryWidget::PlayEnter(float InDelay)
{
	if (InDelay <= 0.f)
	{
		StartEnter();
		return;
	}

	SetVisibility(ESlateVisibility::Hidden);
	GetWorld()->GetTimerManager().SetTimer(EnterDelayTimer, this, &ThisClass::StartEnter, InDelay, false);
}

void UCXTurnOrderEntryWidget::PlayExit()
{
	if (bIsExiting)
	{
		return;
	}

	bIsExiting = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EnterDelayTimer);
	}

	// 아직 보인 적이 없으면 퇴장 연출을 재생할 필요가 없음
	if (!bEnterStarted || nullptr == ExitAnim)
	{
		FinishExit();
		return;
	}

	if (EnterAnim)
	{
		StopAnimation(EnterAnim);
	}

	PlayAnimation(ExitAnim);
}

void UCXTurnOrderEntryWidget::SnapTo(const FVector2D& InPosition)
{
	bIsMoving = false;
	SetRenderTranslation(InPosition);
}

void UCXTurnOrderEntryWidget::MoveTo(const FVector2D& InPosition, float InDuration)
{
	// 같은 목표로 다시 요청된 경우 진행 중인 이동을 처음부터 시작하지 않음
	if (bIsMoving && MoveTarget.Equals(InPosition))
	{
		return;
	}

	const FVector2D CurrentPosition = GetRenderTransform().Translation;
	if (InDuration <= 0.f || CurrentPosition.Equals(InPosition))
	{
		SnapTo(InPosition);
		return;
	}

	MoveStart = CurrentPosition;
	MoveTarget = InPosition;
	MoveElapsed = 0.f;
	MoveDuration = InDuration;
	bIsMoving = true;
}

bool UCXTurnOrderEntryWidget::TickMove(float InDeltaTime)
{
	if (!bIsMoving)
	{
		return false;
	}

	MoveElapsed += InDeltaTime;

	const float Alpha = FMath::Clamp(MoveElapsed / MoveDuration, 0.f, 1.f);
	SetRenderTranslation(FMath::InterpEaseInOut(MoveStart, MoveTarget, Alpha, 2.f));

	bIsMoving = Alpha < 1.f;
	return bIsMoving;
}

void UCXTurnOrderEntryWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EnterDelayTimer);
	}

	Super::NativeDestruct();
}

void UCXTurnOrderEntryWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* InAnimation)
{
	Super::OnAnimationFinished_Implementation(InAnimation);

	if (bIsExiting && InAnimation == ExitAnim.Get())
	{
		FinishExit();
	}
}

void UCXTurnOrderEntryWidget::StartEnter()
{
	bEnterStarted = true;

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (EnterAnim)
	{
		PlayAnimation(EnterAnim);
	}
}

void UCXTurnOrderEntryWidget::FinishExit()
{
	OnExitFinished.ExecuteIfBound(this);
}

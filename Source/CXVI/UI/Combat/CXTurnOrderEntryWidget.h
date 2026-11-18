// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "CXTurnOrderTypes.h"

#include "CXTurnOrderEntryWidget.generated.h"

class UImage;
class ACXCombatCharacterBase;

DECLARE_DELEGATE_OneParam(FCXOnTurnOrderEntryExited, UCXTurnOrderEntryWidget*);

/**
 * 턴 순서 UI 한 칸 (WBP_TurnOrderEntry의 부모)
 * - UCXTurnOrderWidget이 생성 / 제거하고 위치 이동을 요청
 * - 위치는 이 위젯의 RenderTranslation으로 이동. WBP 애니메이션은 안쪽 위젯(ContentRoot 등)만 움직일 것
 * - 애니메이션 3개는 모두 선택 사항. 없으면 연출 없이 바로 처리
 */
UCLASS(Abstract)
class CXVI_API UCXTurnOrderEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetEntryData(const FCXTurnOrderEntryData& InData);

	const ACXCombatCharacterBase* GetCharacter() const { return Character.Get(); }

	// 현재 턴 칸인지. 바뀔 때만 CurrentAnim을 정방향 / 역방향으로 재생
	void SetIsCurrent(bool bInIsCurrent);

	// InDelay 동안 숨겼다가 EnterAnim 재생
	void PlayEnter(float InDelay);

	// ExitAnim이 끝나면 OnExitFinished 호출
	void PlayExit();

	void SnapTo(const FVector2D& InPosition);

	void MoveTo(const FVector2D& InPosition, float InDuration);

	// 이동 중이면 true
	bool TickMove(float InDeltaTime);

protected:
	virtual void NativeDestruct() override;

	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* InAnimation) override;

private:
	void StartEnter();

	void FinishExit();

public:
	FCXOnTurnOrderEntryExited OnExitFinished;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> PortraitImage;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> EnterAnim;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> ExitAnim;

	// 정방향 = 하이라이트 켜기, 역방향 = 끄기
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> CurrentAnim;
	
private:
	TWeakObjectPtr<ACXCombatCharacterBase> Character;

	FTimerHandle EnterDelayTimer;

	bool bIsCurrent = false;

	bool bEnterStarted = false;

	bool bIsExiting = false;

	bool bIsMoving = false;

	FVector2D MoveStart = FVector2D::ZeroVector;

	FVector2D MoveTarget = FVector2D::ZeroVector;

	float MoveElapsed = 0.f;

	float MoveDuration = 0.f;
};

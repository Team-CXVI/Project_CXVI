// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"

#include "CXJustTimingZoneWidget.generated.h"

class UOverlay;
class UCXJustTimingComponent;
class UCXJustTimingGaugeWidget;
struct FCXJustTimingMessage;

// 결과를 기다리는 게이지 하나 (구간 = Source + TimingKey)
struct FCXJustTimingGaugeEntry
{
	TWeakObjectPtr<UCXJustTimingComponent> Source;

	// x = 타격번호, y = 판정번호
	FIntPoint TimingKey = FIntPoint(INDEX_NONE, INDEX_NONE);

	TWeakObjectPtr<UCXJustTimingGaugeWidget> Gauge;
};

/**
 * 전투 HUD의 저스트 타이밍 표시 영역 (WBP_JustTimingZone의 부모, WBP_CombatHUD에 배치)
 * - Message.Combat.JustTiming 수신
 *   - Opened : 게이지 생성 → GaugeOverlay 끝에 추가(가장 위) → RandomOffsetRange 안의 랜덤 위치
 *   - Succeeded / Failed : 해당 게이지에 결과 전달 (애니메이션 후 게이지가 스스로 제거)
 *   - Cleared : 해당 게이지 바로 제거
 * - 구독 기간은 Construct ~ Destruct. Destruct 때 남은 게이지 제거
 */
UCLASS(Abstract)
class CXVI_API UCXJustTimingZoneWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

private:
	void HandleJustTiming(const FGameplayTag& InChannel, const FCXJustTimingMessage& InMessage);

	void AddGauge(const FCXJustTimingMessage& InMessage);

	// 메시지의 구간에 해당하는 게이지를 목록에서 빼서 반환 (없으면 nullptr)
	UCXJustTimingGaugeWidget* TakeGauge(const FCXJustTimingMessage& InMessage);

	void ClearGauges();

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> GaugeOverlay;

	UPROPERTY(EditDefaultsOnly, Category = "JustTiming")
	TSubclassOf<UCXJustTimingGaugeWidget> GaugeWidgetClass;

	// 존 중앙에서 X / Y 각각 -Range ~ +Range 안의 랜덤 위치 (화면 픽셀)
	UPROPERTY(EditDefaultsOnly, Category = "JustTiming")
	FVector2D RandomOffsetRange = FVector2D::ZeroVector;

	// 결과를 기다리는 게이지들
	TArray<FCXJustTimingGaugeEntry> ActiveGauges;

	FCXGameplayMessageListenerHandle JustTimingListener;
	
};

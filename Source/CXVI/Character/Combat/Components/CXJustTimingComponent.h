// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "Engine/TimerHandle.h"
#include "Character/Combat/Data/CXJustTimingData.h"

#include "CXJustTimingComponent.generated.h"

class UAnimMontage;
class USkeletalMeshComponent;
enum class ECXJustTimingEvent : uint8;

struct FCXJustTimingWindow
{
	double StartTimeSeconds = 0.0;
	FTimerHandle TimeoutHandle;
	bool bActive = true;
	bool bSuccess = false;
};
/**
 * 저스트 타이밍 판정 (전투 플레이어 캐릭터에 부착)
 * - 판정 구간 = 몽타주의 CX Just Timing Window 노티파이 스테이트 (BeginWindow / EndWindow)
 * - 진행도 = (몽타주 위치 - 구간 시작) / 구간 길이, 성공 범위 안에서 입력하면 성공
 * - 구간 시작 / 결과 / 취소를 Message.Combat.JustTiming 으로 알림 (UI는 이 메시지 + GetWindowProgress로 표시)
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CXVI_API UCXJustTimingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    void BeginWindow(int32 InHitNumber, int32 InTimingIndex);
    void HandleInput();
    bool ConsumeSuccess(int32 InHitNumber, int32 RequiredCount = 1);
    void ResetTiming();

    bool GetWindowProgress(int32 InHitNumber, int32 InTimingIndex, float& OutProgress) const;

protected:
    virtual void BeginPlay() override;
    void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void EndWindow(FIntPoint TimingKey);
    void ResolveWindow(const FIntPoint& TimingKey, bool bSucceeded, float Progress);
    void ClearWindowTimer(FCXJustTimingWindow& TimingWindow);

    void BroadcastTimingMessage(ECXJustTimingEvent TimingEvent, const FIntPoint& TimingKey, float Progress);

    // X는 타격 또는 연계 단계 번호, Y는 그 안의 판정 번호다.
    TMap<FIntPoint, FCXJustTimingWindow> Windows;

    UPROPERTY(EditDefaultsOnly, Category = "Just Timing")
    TObjectPtr<UCXJustTimingData> JustTimingData;

    FCXJustTimingSettings TimingSettings;


};
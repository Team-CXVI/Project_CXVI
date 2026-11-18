// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Combat/Components/CXJustTimingComponent.h"

#include "Foundation/Combat/CombatMessage/CXCombatMessageData.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"
#include "Tags/CXGameplayTags.h"

#include "Engine/World.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXJustTimingComponent)

namespace
{
    constexpr float TimingWindowDurationSeconds = 0.6f;
    constexpr float TimingSuccessStartRatio = 0.75f;
    constexpr float TimingSuccessEndRatio = 1.0f;
}

void UCXJustTimingComponent::BeginWindow(int32 InHitNumber, int32 InTimingIndex)
{
    UWorld* TimingWorld = GetWorld();
    if (!TimingWorld || InHitNumber < 1 || InTimingIndex < 1)
    {
        return;
    }

    const FIntPoint TimingKey(InHitNumber, InTimingIndex);
    if (Windows.Contains(TimingKey))
    {
        return;
    }

    FCXJustTimingWindow& TimingWindow = Windows.Add(TimingKey);
    TimingWindow.StartTimeSeconds = TimingWorld->GetTimeSeconds();

    const FTimerDelegate TimeoutDelegate = FTimerDelegate::CreateUObject(
        this, &UCXJustTimingComponent::EndWindow, TimingKey);

    TimingWorld->GetTimerManager().SetTimer(
        TimingWindow.TimeoutHandle, TimeoutDelegate,
        TimingWindowDurationSeconds, false);

    BroadcastTimingMessage(ECXJustTimingEvent::Opened, TimingKey, 0.f);
}

void UCXJustTimingComponent::HandleInput()
{
    const UWorld* TimingWorld = GetWorld();
    if (!TimingWorld)
    {
        return;
    }

    const double NowSeconds = TimingWorld->GetTimeSeconds();
    FIntPoint SelectedKey(INDEX_NONE, INDEX_NONE);
    double SelectedElapsed = -1.0;

    for (const auto& Pair : Windows)
    {
        const FCXJustTimingWindow& TimingWindow = Pair.Value;
        const double Elapsed = NowSeconds - TimingWindow.StartTimeSeconds;

        if (!TimingWindow.bActive || Elapsed < 0.0 ||
            Elapsed >= TimingWindowDurationSeconds)
        {
            continue;
        }

        // 판정이 겹치면 먼저 닫히는 구간 하나에만 입력을 적용한다.
        const bool bEarlierKey = Pair.Key.X < SelectedKey.X ||
            (Pair.Key.X == SelectedKey.X && Pair.Key.Y < SelectedKey.Y);

        if (Elapsed > SelectedElapsed ||
            (Elapsed == SelectedElapsed && bEarlierKey))
        {
            SelectedKey = Pair.Key;
            SelectedElapsed = Elapsed;
        }
    }

    if (SelectedKey.X == INDEX_NONE)
    {
        return;
    }

    const float Progress =
        static_cast<float>(SelectedElapsed / TimingWindowDurationSeconds);
    const bool bSucceeded =
        Progress >= TimingSuccessStartRatio && Progress <= TimingSuccessEndRatio;

    ResolveWindow(SelectedKey, bSucceeded, Progress);
}

bool UCXJustTimingComponent::ConsumeSuccess(int32 InHitNumber, int32 RequiredCount)
{
    RequiredCount = FMath::Max(1, RequiredCount);

    int32 RegisteredCount = 0;
    int32 SuccessCount = 0;
    TArray<TPair<FIntPoint, float>> UnresolvedWindows;

    for (auto It = Windows.CreateIterator(); It; ++It)
    {
        if (It.Key().X != InHitNumber)
        {
            continue;
        }

        ++RegisteredCount;
        FCXJustTimingWindow& TimingWindow = It.Value();

        if (TimingWindow.bActive)
        {
            float Progress = 1.f;
            GetWindowProgress(It.Key().X, It.Key().Y, Progress);
            UnresolvedWindows.Emplace(It.Key(), Progress);
        }
        else if (TimingWindow.bSuccess && It.Key().Y <= RequiredCount)
        {
            ++SuccessCount;
        }

        ClearWindowTimer(TimingWindow);
        It.RemoveCurrent();
    }

    // 필요한 판정이 모두 등록됐고 전부 성공했을 때만 성공이다.
    const bool bAllSucceeded =
        RegisteredCount == RequiredCount && SuccessCount == RequiredCount;

    UE_LOG(LogTemp, Display, TEXT("[저스트] %d타 최종: %d/%d 성공"),
        InHitNumber, SuccessCount, RequiredCount);

    // 기록을 먼저 지운 뒤 UI에 결과를 보낸다.
    for (const auto& Pair : UnresolvedWindows)
    {
        BroadcastTimingMessage(ECXJustTimingEvent::Failed, Pair.Key, Pair.Value);
    }

    return bAllSucceeded;
}

void UCXJustTimingComponent::ResetTiming()
{
    TArray<FIntPoint> UnresolvedKeys;

    for (auto& Pair : Windows)
    {
        ClearWindowTimer(Pair.Value);
        if (Pair.Value.bActive)
        {
            UnresolvedKeys.Add(Pair.Key);
        }
    }

    Windows.Reset();

    for (const FIntPoint& TimingKey : UnresolvedKeys)
    {
        BroadcastTimingMessage(ECXJustTimingEvent::Cleared, TimingKey, 0.f);
    }
}

bool UCXJustTimingComponent::GetWindowProgress(int32 InHitNumber, int32 InTimingIndex, float& OutProgress) const
{
    const UWorld* TimingWorld = GetWorld();
    const FCXJustTimingWindow* TimingWindow =
        Windows.Find(FIntPoint(InHitNumber, InTimingIndex));

    if (!TimingWorld || !TimingWindow || !TimingWindow->bActive)
    {
        return false;
    }

    const double Elapsed = TimingWorld->GetTimeSeconds() - TimingWindow->StartTimeSeconds;
    OutProgress = static_cast<float>(
        FMath::Clamp(Elapsed / TimingWindowDurationSeconds, 0.0, 1.0));

    return true;
}

void UCXJustTimingComponent::BeginPlay()
{
    Super::BeginPlay();

    if (IsValid(JustTimingData))
    {
        TimingSettings = JustTimingData->Settings;
    }

    TimingSettings.WindowDurationSeconds =
        FMath::Max(0.01f, TimingSettings.WindowDurationSeconds);
    TimingSettings.SuccessStartRatio =
        FMath::Clamp(TimingSettings.SuccessStartRatio, 0.f, 1.f);
    TimingSettings.SuccessEndRatio =
        FMath::Clamp(TimingSettings.SuccessEndRatio, TimingSettings.SuccessStartRatio, 1.f);
}

void UCXJustTimingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ResetTiming();
    Super::EndPlay(EndPlayReason);
}

void UCXJustTimingComponent::EndWindow(FIntPoint TimingKey)
{
    // 제한 시간 안에 입력이 없으면 실패로 확정한다.
    ResolveWindow(TimingKey, false, 1.f);
}

void UCXJustTimingComponent::ResolveWindow(const FIntPoint& TimingKey, bool bSucceeded, float Progress)
{
    FCXJustTimingWindow* TimingWindow = Windows.Find(TimingKey);
    if (!TimingWindow || !TimingWindow->bActive)
    {
        return;
    }

    // 결과는 즉시 확정하고, 타격이나 연계가 읽을 때까지 보관한다.
    TimingWindow->bActive = false;
    TimingWindow->bSuccess = bSucceeded;
    ClearWindowTimer(*TimingWindow);

    UE_LOG(LogTemp, Display, TEXT("[저스트] %d타 / 판정 %d: 진행도 %.2f / %s"),
        TimingKey.X, TimingKey.Y, Progress,
        bSucceeded ? TEXT("성공") : TEXT("실패"));

    BroadcastTimingMessage(
        bSucceeded ? ECXJustTimingEvent::Succeeded : ECXJustTimingEvent::Failed,
        TimingKey, Progress);
}

void UCXJustTimingComponent::ClearWindowTimer(FCXJustTimingWindow& TimingWindow)
{
    if (UWorld* TimingWorld = GetWorld())
    {
        TimingWorld->GetTimerManager().ClearTimer(TimingWindow.TimeoutHandle);
    }
}

void UCXJustTimingComponent::BroadcastTimingMessage(ECXJustTimingEvent TimingEvent, const FIntPoint& TimingKey, float Progress)
{
    if (!UCXGameplayMessageSubsystem::HasInstance(this))
    {
        return;
    }

    FCXJustTimingMessage Message;
    Message.Event = TimingEvent;
    Message.Source = this;
    Message.HitNumber = TimingKey.X;
    Message.TimingIndex = TimingKey.Y;
    Message.SuccessStartRatio = TimingSuccessStartRatio;
    Message.SuccessEndRatio = TimingSuccessEndRatio;
    Message.Progress = Progress;

    UCXGameplayMessageSubsystem::Get(this).BroadcastMessage(
        CXVIGameplayTags::Message_Combat_JustTiming, Message);
}

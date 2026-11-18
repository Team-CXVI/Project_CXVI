// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXJustTimingGaugeWidget.h"

#include "Character/Combat/Components/CXJustTimingComponent.h"
#include "UI/CXUIManagerSubsystem.h"

#include <Animation/WidgetAnimation.h>
#include <Components/Image.h>
#include <Materials/MaterialInstanceDynamic.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXJustTimingGaugeWidget)

void UCXJustTimingGaugeWidget::BeginGauge(UCXJustTimingComponent* InSource, const FIntPoint& InTimingKey, float InSuccessStartRatio, float InSuccessEndRatio)
{
	Source = InSource;
	TimingKey = InTimingKey;

	SetGaugeParameter(SuccessStartParamName, InSuccessStartRatio);
	SetGaugeParameter(SuccessEndParamName, InSuccessEndRatio);
	SetGaugeParameter(PercentParamName, 0.f);

	bTracking = true;
}

void UCXJustTimingGaugeWidget::ShowResult(bool bInSuccess, float InProgress)
{
	bTracking = false;

	// 결과가 정해진 순간의 위치로 고정
	SetGaugeParameter(PercentParamName, InProgress);

	PlayingAnim = bInSuccess ? SuccessAnim : FailAnim;
	if (!IsValid(PlayingAnim))
	{
		UE_LOG(LogCXUI, Warning, TEXT("[JustTiming] 결과 애니메이션 없음 (%s의 SuccessAnim / FailAnim 확인)"), *GetNameSafe(GetClass()));
		RemoveFromParent();
		return;
	}

	PlayAnimation(PlayingAnim);
}

void UCXJustTimingGaugeWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Brush에 지정한 머티리얼로 Dynamic Material Instance를 한 번 만들어 Brush에 적용 (이후 파라미터만 갱신)
	GaugeMaterial = GaugeImage ? GaugeImage->GetDynamicMaterial() : nullptr;
	if (nullptr == GaugeMaterial)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[JustTiming] %s의 GaugeImage Brush에 머티리얼이 없어 게이지를 표시하지 않습니다."), *GetNameSafe(GetClass()));
	}

	SetGaugeParameter(PercentParamName, 0.f);
}

void UCXJustTimingGaugeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bTracking)
	{
		return;
	}

	const UCXJustTimingComponent* Timing = Source.Get();

	float Progress = 0.f;

	// 조회 실패(몽타주 중단 등)면 마지막 값 유지. 제거는 결과 / 취소 메시지에서
	if (IsValid(Timing) && Timing->GetWindowProgress(TimingKey.X, TimingKey.Y, Progress))
	{
		SetGaugeParameter(PercentParamName, Progress);
	}
}

void UCXJustTimingGaugeWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* InAnimation)
{
	Super::OnAnimationFinished_Implementation(InAnimation);

	if (InAnimation == PlayingAnim)
	{
		PlayingAnim = nullptr;
		RemoveFromParent();
	}
}

void UCXJustTimingGaugeWidget::SetGaugeParameter(FName InParamName, float InValue)
{
	if (GaugeMaterial)
	{
		GaugeMaterial->SetScalarParameterValue(InParamName, InValue);
	}
}

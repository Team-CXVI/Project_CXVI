// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"

#include "CXJustTimingGaugeWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UWidgetAnimation;
class UCXJustTimingComponent;

/**
 * 저스트 타이밍 판정 구간 하나의 게이지 (UCXJustTimingZoneWidget이 생성해서 Overlay에 추가)
 * - BeginGauge : 성공 구간을 머티리얼에 전달하고 진행도 갱신 시작
 * - 진행 중 : 매 프레임 판정 컴포넌트의 GetWindowProgress → 머티리얼 Percent
 * - ShowResult : 갱신을 멈추고 결과 순간의 진행도로 고정 → SuccessAnim / FailAnim 재생 → 끝나면 RemoveFromParent
 * - GaugeImage 머티리얼 Scalar Parameter (모두 0 ~ 1) : Percent(진행도), SuccessStart / SuccessEnd(성공 구간)
 * - 연출은 WBP 애니메이션 담당. 애니메이션은 루트가 아닌 자식에 걸 것 (루트 Render Translation = 존 안의 오프셋)
 */
UCLASS(Abstract)
class CXVI_API UCXJustTimingGaugeWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	void BeginGauge(UCXJustTimingComponent* InSource, const FIntPoint& InTimingKey, float InSuccessStartRatio, float InSuccessEndRatio);

	void ShowResult(bool bInSuccess, float InProgress);

protected:
	virtual void NativeOnInitialized() override;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* InAnimation) override;

private:
	// 머티리얼이 없으면 무시
	void SetGaugeParameter(FName InParamName, float InValue);
	
private:
	// Brush에 Percent / SuccessStart / SuccessEnd 파라미터를 가진 UI 머티리얼 지정
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> GaugeImage;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> SuccessAnim;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> FailAnim;

	// GaugeImage의 Dynamic Material Instance (NativeOnInitialized에서 생성)
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GaugeMaterial;

	UPROPERTY(VisibleDefaultsOnly, Category = "CXVI|MIDParam")
	FName PercentParamName = TEXT("Percent");

	UPROPERTY(VisibleDefaultsOnly, Category = "CXVI|MIDParam")
	FName SuccessStartParamName = TEXT("SuccessStart");

	UPROPERTY(VisibleDefaultsOnly, Category = "CXVI|MIDParam")
	FName SuccessEndParamName = TEXT("SuccessEnd");

	// 지금 재생 중인 결과 애니메이션 (끝나면 제거)
	UPROPERTY(Transient)
	TObjectPtr<UWidgetAnimation> PlayingAnim;

	TWeakObjectPtr<UCXJustTimingComponent> Source;

	// x = 타격번호, y = 판정번호
	FIntPoint TimingKey = FIntPoint(INDEX_NONE, INDEX_NONE);

	// 결과 전까지 true (진행도 갱신)
	bool bTracking = false;
};

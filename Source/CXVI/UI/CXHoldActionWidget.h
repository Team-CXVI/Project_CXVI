// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Input/UIActionBindingHandle.h"

#include "CXHoldActionWidget.generated.h"

class UCXInputIconWidget;
class UImage;
class UInputAction;
class UMaterialInstanceDynamic;
class UTextBlock;

/**
 * 키를 일정 시간 누르고 있으면 실행되는 범용 홀드 입력 칸 (실제 동작은 자식에서 구현)
 * - 게이지가 다 차면 OnHoldCompleted 호출 → C++ 자식은 OnHoldCompleted_Implementation, WBP는 On Hold Completed를 오버라이딩
 * - SetHoldActionActive
 *   - true : 입력 바인딩 등록 + 표시
 *   - false : 입력 바인딩 해제 + 게이지 0 + Collapsed (생성 직후 상태)
 *   - 숨기기만 하면 CommonUI가 진행 중인 홀드를 취소하지 않아, 키를 누른 채 다시 표시될 때 바로 발동할 수 있음
 *     새로 등록한 바인딩은 진행 중인 홀드가 없어서 키를 놓았다가 다시 눌러야 시작됨
 * - 홀드 시간 = HoldAction의 Hold 트리거 Hold Time Threshold
 *   - 필요 CVar (DefaultEngine.ini [ConsoleVariables])
 *     CommonUI.bEnhancedInputDynamicallyResolveHoldKeysForInputAction=1, CommonUI.AlwaysWaitForHoldActionToComplete=1
 *   - 위 설정이 하나라도 빠지면 누르는 즉시 실행될 수 있어서 바인딩하지 않고 Error 로그
 * - 진행도(0~1) → HoldGaugeImage 머티리얼의 Scalar Parameter "Percent"
 * - 키 아이콘 → UI Mapping Context(IMC_UI)에서 HoldAction에 매핑된 키를 찾아 KeyIcon에 전달
 */
UCLASS()
class CXVI_API UCXHoldActionWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
	
public:
	UFUNCTION(BlueprintCallable, Category = "CXVI|HoldAction")
	void SetHoldActionActive(bool bInActive);

	UFUNCTION(BlueprintPure, Category = "CXVI|HoldAction")
	bool IsHoldActionActive() const { return bHoldActionActive; }

protected:
	virtual void NativeOnInitialized() override;

	virtual void NativePreConstruct() override;

	virtual void NativeDestruct() override;

	// 게이지가 다 찼을 때 호출. 이 안에서 SetHoldActionActive(false)를 호출해도 안전함
	UFUNCTION(BlueprintNativeEvent, Category = "CXVI|HoldAction")
	void OnHoldCompleted();

	virtual void OnHoldCompleted_Implementation() {}

private:
	// 홀드 입력에 필요한 설정(IA Hold 트리거, CVar)이 모두 있는지 확인. 빠진 항목은 Error 로그
	bool IsHoldInputConfigured() const;

	void RegisterHoldBinding();

	void UnregisterHoldBinding();

	// 게이지 머티리얼의 Percent 갱신 (머티리얼이 없으면 무시)
	void SetGaugePercent(float InPercent);

	// IMC_UI에서 HoldAction의 키를 찾아 KeyIcon에 전달
	void RefreshKeyIcon();

protected:
	// 칸마다 지정 (Hold 트리거 필요, 키는 IMC_UI에 매핑해야 키 조회 / 입력 처리가 됨)
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> HoldAction;

	// 설명 (칸마다 지정. LabelText가 없는 WBP에서는 사용하지 않음)
	UPROPERTY(EditAnywhere, Category = "Hint")
	FText Label;

private:
	// Brush에 Percent 파라미터를 가진 UI 머티리얼 지정
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> HoldGaugeImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXInputIconWidget> KeyIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LabelText;

	// HoldGaugeImage의 Dynamic Material Instance (NativeOnInitialized에서 생성)
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HoldGaugeMaterial;

	// 게이지 머티리얼의 진행도 Scalar Parameter 이름 (0 ~ 1)
	UPROPERTY(VisibleDefaultsOnly, Category = "CXVI|MIDParam")
	FName GaugePercentParamName = TEXT("Percent");

	FUIActionBindingHandle HoldBinding;

	bool bHoldInputConfigured = false;

	bool bHoldActionActive = false;
	
};

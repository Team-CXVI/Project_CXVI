// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"

#include "CXPotionSlotWidget.generated.h"

class UImage;
class UInputAction;
class UMaterialInstanceDynamic;
class UTexture2D;

/**
 * 필드 HUD 포션 칸 (WBP_PotionSlot의 부모, WBP_FieldHUD 안에 배치)
 * - UsePotionAction을 Game 모드 UI 액션(CommonUI Hold)으로 바인딩
 *   - 홀드 시간 = IA의 Hold 트리거 Hold Time Threshold
 *   - 필요 CVar (DefaultEngine.ini [ConsoleVariables])
 *     CommonUI.bEnhancedInputDynamicallyResolveHoldKeysForInputAction=1 : IA의 Hold 트리거를 CommonUI Hold로 인식
 *     CommonUI.AlwaysWaitForHoldActionToComplete=1 : 짧게 탭했을 때 일반 Press로 바뀌어 즉시 사용되는 것을 막음
 *   - 위 설정이 하나라도 빠지면 누르는 즉시 사용될 수 있어서 바인딩하지 않고 Error 로그
 * - 홀드 진행도(0~1) → HoldGaugeImage 머티리얼의 Scalar Parameter "Percent". 놓거나 사용이 끝나면 CommonUI가 0을 보냄
 *   - 게이지 모양(원 / 사각 테두리 등)은 WBP에서 HoldGaugeImage Brush에 지정한 UI 머티리얼이 정함 (C++는 Percent만 넣음)
 * - 필드에서 쓸 수 있는 아이템은 포션뿐이라 사용 처리는 이 위젯이 UserData를 직접 호출 (전투 포션은 GE로 별도 처리)
 *   - 사용 가능 : UCXUserDataSubsystem::GetItemCount(PotionItemId) > 0. 0이면 게이지를 채우지 않음
 *   - 게이지가 다 차면 UCXUserDataSubsystem::RemoveItem(PotionItemId, 1) → InventoryChanged로 VM 갱신
 *   - TODO : 회복 효과 (필드 = SetMemberHealth, 대상 / 회복량 미정)
 * - 포션 ItemId : UCXGameplayDataSettings::PotionItemId
 * - 아이콘 / 수량 / 사용 가능 표시는 WBP View Bindings(CXPotionViewModel)
 *   아이콘은 Soft 참조라 C++에서 비동기 로드 : WBP에서 Icon → SetIcon 바인딩
 */
UCLASS(Abstract)
class CXVI_API UCXPotionSlotWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	

public:
	UFUNCTION(BlueprintCallable, Category = "CXVI|Potion")
	void SetIcon(TSoftObjectPtr<UTexture2D> InIcon);

protected:
	virtual void NativeOnInitialized() override;

private:
	// 홀드 입력에 필요한 설정(IA Hold 트리거, CVar)이 모두 있는지 확인. 빠진 항목은 Error 로그
	bool IsHoldInputConfigured() const;

	// 게이지 머티리얼의 Percent 갱신 (머티리얼이 없으면 무시)
	void SetGaugePercent(float InPercent);

	//void HandleHoldProgressed(float InPercent);

	void HandleUsePotion();

protected:
	// 포션 사용 입력 (Hold 트리거 필요, 키는 IMC_UI에 매핑해야 키 조회 / 입력 처리가 됨)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> UsePotionAction;

private:
	// 물약 아이템 아이콘
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;

	// Brush에 Percent 파라미터를 가진 UI 머티리얼 지정
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> HoldGaugeImage;

	// HoldGaugeImage의 Dynamic Material Instance (NativeOnInitialized에서 생성)
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HoldGaugeMaterial;

	// 게이지 머티리얼의 진행도 Scalar Parameter 이름 (0 ~ 1)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "CXVI|MIDParam", meta = (AllowPrivateAccess = true))
	FName GaugePercentParamName = TEXT("Percent");
};

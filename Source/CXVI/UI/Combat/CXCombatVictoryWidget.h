// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/CXActivatableWidget.h"

#include "CXCombatVictoryWidget.generated.h"

class UInputAction;
class UCXInputHintWidget;
class UCXCountUpTextBlock;
class UPanelWidget;
class UCXCombatVictoryMemberWidget;
struct FCXCombatResult;

/**
 * 전투 승리 화면 베이스 (WBP_CombatVictory의 부모)
 * - 전투 Scene이 전투 HUD를 닫고 UI.Layer.GameMenu에 Push
 * - Push할 때 SetResult로 전투 결과를 받아 획득 경험치(숫자)와 파티원별 결과(초상화, 현재 레벨, 레벨업 표시, HP)를 채움
 * - 획득 경험치는 0으로 시작하고 FrameEnterAnim의 ExpText.CountAlpha 트랙(0 -> 1)을 따라 최종 값까지 올라감
 * - 활성화되어 있는 동안 게임 일시정지
 * - FrameEnterAnim(프레임 + 전투 승리 문구)이 끝나면 ConfirmHint를 표시하고 확인 입력을 받음
 * - 확인 입력은 포커스 없이 UI 액션으로 처리 (ConfirmAction의 키는 IMC_UI에 매핑되어 있어야 함)
 * - 확인 : 위젯을 닫아 일시정지를 푼 뒤 콜백 실행 (Scene 전환의 페이드는 타이머라 일시정지 중에는 진행되지 않음)
 * - Stack이 위젯을 재사용하므로 콜백은 비활성화될 때, 파티원 결과는 SetResult에서 정리
 */
UCLASS(Abstract)
class CXVI_API UCXCombatVictoryWidget : public UCXActivatableWidget
{
	GENERATED_BODY()
	
public:
	UCXCombatVictoryWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 확인 입력으로 닫힌 뒤 실행할 콜백
	void SetOnConfirmed(FSimpleDelegate InOnConfirmed);

	// 획득 경험치와 파티원별 결과 채우기
	void SetResult(const FCXCombatResult& InResult);

protected:
	virtual void NativeOnInitialized() override;

	virtual void NativeOnActivated() override;

	virtual void NativeOnDeactivated() override;

	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* InAnimation) override;

private:
	void HandleConfirm();

protected:
	// 확인 입력
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ConfirmAction;

	// 파티원 결과 위젯 클래스 (WBP_CombatVictoryMember)
	UPROPERTY(EditDefaultsOnly, Category = "Result")
	TSubclassOf<UCXCombatVictoryMemberWidget> MemberClass;

	// 획득 경험치 (숫자만). 0부터 올라가는 연출은 FrameEnterAnim의 CountAlpha 트랙이 담당
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXCountUpTextBlock> ExpText;

	// 파티원 결과 위젯을 담는 패널
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> MemberBox;

	// 화면 오른쪽 하단의 확인 안내 (문구 + 키 아이콘)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXInputHintWidget> ConfirmHint;

	// 프레임 + 전투 승리 문구 등장 애니메이션
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> FrameEnterAnim;
	
private:
	FSimpleDelegate OnConfirmed;

	// 안내가 표시된 뒤에만 확인 입력을 받음
	// TODO : 추후에 전투 결과 표시 연출이 전부 들어갔을 때 확인 버튼으로 결과 연출을 날리는 것도 고려
	bool bCanConfirm = false;

};

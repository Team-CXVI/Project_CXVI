// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/CXActivatableWidget.h"

#include "CXCombatDefeatWidget.generated.h"

class UCXButtonBase;

/**
 * 전투 패배 화면 베이스 (WBP_CombatDefeat의 부모)
 * - 전투 Scene이 전투 HUD를 닫고 UI.Layer.GameMenu에 Push
 * - 입력 모드 Menu. 활성화되어 있는 동안 게임 일시정지
 * - IntroAnim(얼룩 -> 패배 문구 -> 문구 이동 -> 선택지 순서대로 등장)이 끝나야 선택지 입력을 받음
 * - 마지막 지점에서 : 자동 저장 슬롯을 불러와 저장된 위치로 이동
 * - 로드 하기 : Message.UI.OpenScreen으로 저장 슬롯 화면을 불러오기 모드로 열기 (뒤로가기로 이 화면에 돌아옴)
 * - 타이틀로 : 닫아서 정지 해제 후 타이틀 이동
 * - 저장 파일이 없으면 해당 선택지 비활성화. 뒤로가기로 닫을 수 없음
 */
UCLASS()
class CXVI_API UCXCombatDefeatWidget : public UCXActivatableWidget
{
	GENERATED_BODY()

public:
	UCXCombatDefeatWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativeOnInitialized() override;

	virtual void NativeOnActivated() override;

	virtual void NativeOnDeactivated() override;

	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* InAnimation) override;

private:
	// 저장 파일 유무에 맞춰 선택지 활성화 갱신
	void RefreshButtons();

	void HandleLastPointClicked();

	void HandleLoadClicked();

	void HandleTitleClicked();
	
protected:
	// 마지막 지점에서 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXButtonBase> LastPointButton;

	// 로드 하기 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXButtonBase> LoadButton;

	// 타이틀로 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCXButtonBase> TitleButton;

	// 얼룩 -> 패배 문구 -> 문구 이동 -> 선택지 등장 애니메이션
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> IntroAnim;

private:
	// IntroAnim이 끝난 뒤에만 선택지 입력을 받음
	bool bMenuReady = false;
};

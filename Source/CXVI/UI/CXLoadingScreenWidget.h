// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "CXLoadingScreenWidget.generated.h"

/**
 * 레벨 이동 로딩 화면
 * - UCXUIManagerSubsystem이 Message.Level.Loading을 받아 레이아웃 밖(LayoutZOrder + 1)에 생성 / 제거
 * - 레이어 스택에 넣지 않으므로 UCommonUserWidget 사용
 * - 입력을 받지 않음 (HitTestInvisible)
 * - 표시 중에는 게임이 일시정지 상태. 움직이는 표시는 LoopAnim(위젯 애니메이션)으로 처리
 * - TODO : 진행률 / 팁 등 표시 내용은 디자인이 정해지면 추가
 */
UCLASS(Abstract)
class CXVI_API UCXLoadingScreenWidget : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

	virtual void NativeConstruct() override;

protected:
	// 표시되는 동안 반복 재생 (없으면 재생하지 않음)
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> LoopAnim;
	
};

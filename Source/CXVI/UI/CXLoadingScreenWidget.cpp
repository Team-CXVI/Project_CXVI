// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXLoadingScreenWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXLoadingScreenWidget)

void UCXLoadingScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 아래 레이아웃(불투명도 0)의 입력을 가로채지 않도록 클릭 / 포커스 대상에서 제외
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCXLoadingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LoopAnim)
	{
		// NumLoopsToPlay = 0 : 무한 반복
		PlayAnimation(LoopAnim, 0.f, 0);
	}
}

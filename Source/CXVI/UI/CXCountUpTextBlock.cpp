// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXCountUpTextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCountUpTextBlock)

void UCXCountUpTextBlock::SetTargetValue(int32 InTargetValue)
{
	TargetValue = InTargetValue;

	RefreshCountText();
}

void UCXCountUpTextBlock::SetCountAlpha(float InCountAlpha)
{
	// 커브 보간으로 범위를 넘는 키 값이 들어와도 0 ~ 최종 값 안에서만 표시
	CountAlpha = FMath::Clamp(InCountAlpha, 0.f, 1.f);

	RefreshCountText();
}

void UCXCountUpTextBlock::RefreshCountText()
{
	const int32 DisplayValue = FMath::RoundToInt(TargetValue * CountAlpha);

	SetText(FText::AsNumber(DisplayValue));
}

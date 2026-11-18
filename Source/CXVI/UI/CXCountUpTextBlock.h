// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonTextBlock.h"

#include "CXCountUpTextBlock.generated.h"

/**
 * 0부터 목표 값까지 올라가는 숫자 텍스트
 * - 코드에서 SetTargetValue로 최종 값을 지정
 * - 진행 비율(CountAlpha, 0 ~ 1)은 UMG 애니메이션 트랙이 움직임 (시작 시점 / 길이 / 곡선은 WBP 애니메이션에서 결정)
 * - CountAlpha가 바뀔 때마다 Setter(SetCountAlpha)가 호출되어 TargetValue * CountAlpha를 반올림해 표시
 * - UMG 애니메이션은 게임 일시정지 중에도 재생되므로 일시정지 화면에서도 동작
 * - 텍스트 블록을 직접 상속하므로 별도 WBP 없이 팔레트에서 배치
 */
UCLASS()
class CXVI_API UCXCountUpTextBlock : public UCommonTextBlock
{
	GENERATED_BODY()

public:
	// 최종 값 지정. 현재 진행 비율 기준으로 표시를 갱신
	void SetTargetValue(int32 InTargetValue);

	// 진행 비율 지정 (0 ~ 1). 애니메이션 트랙도 이 함수를 거침
	void SetCountAlpha(float InCountAlpha);

private:
	void RefreshCountText();

protected:
	// 진행 비율 (0 ~ 1). UMG 애니메이션 트랙으로 움직임
	UPROPERTY(EditAnywhere, Interp, Setter, Category = "Count Up", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CountAlpha = 1.f;

private:
	// 최종 값
	int32 TargetValue = 0;
};

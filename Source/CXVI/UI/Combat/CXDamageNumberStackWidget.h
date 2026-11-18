// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Foundation/Combat/CombatMessage/CXCombatMessageData.h"

#include "CXDamageNumberStackWidget.generated.h"

class UOverlay;
class UCXDamageNumberWidget;

/**
 * 한 대상의 데미지 숫자 컨테이너
 * - AddDamageResult : 숫자 위젯을 만들어 NumberOverlay 끝에 추가 → 나중 숫자가 위에 그려짐
 * - 숫자 위젯 루트에 RandomOffsetRange 안의 랜덤 위치 적용 (0이면 한 점)
 * - 숫자 위젯은 애니메이션이 끝나면 스스로 빠짐. 컨테이너는 대상이 사라질 때까지 재사용
 */
UCLASS(Abstract)
class CXVI_API UCXDamageNumberStackWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void AddDamageResult(ECXDamageResult InResult, float InAmount);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> NumberOverlay;

	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	TSubclassOf<UCXDamageNumberWidget> NumberWidgetClass;

	// 기준점에서 X / Y 각각 -Range ~ +Range 안의 랜덤 위치 (화면 픽셀)
	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	FVector2D RandomOffsetRange = FVector2D::ZeroVector;
};
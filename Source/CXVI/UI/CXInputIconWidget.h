// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "CommonInputTypeEnum.h"

#include "CXInputIconWidget.generated.h"

class UImage;

/**
 * 버튼 아이콘
 * - 기믹 상호 작용에 표시하는 입력 키 한 개를 표시하는 아이콘 위젯
 * - 아이콘은 Common Input Settings에 등록된 Controller Data에서 찾음
 */
UCLASS(Abstract)
class CXVI_API UCXInputIconWidget : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

private:
	void HandleInputMethodChanged(ECommonInputType InInputType);

	void RefreshIcon(ECommonInputType InInputType);

protected:
	// 키보드 / 마우스 사용 시 표시할 키
	UPROPERTY(EditAnywhere, Category = "InputIcon")
	FKey KeyboardKey;
	
	// 게임패드 사용 중일 때 표시할 키
	UPROPERTY(EditAnywhere, Category = "InputIcon")
	FKey GamepadKey;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> InputIcon;
};

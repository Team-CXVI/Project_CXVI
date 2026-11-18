// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "CommonInputTypeEnum.h"

#include "CXInputHintWidget.generated.h"

class UTextBlock;
class UHorizontalBox;

/**
 * 버튼 안내 한 칸 (아이콘 + 설명)
 * - 현재 입력 장치(키보드/마우스, 게임패드)의 키 목록으로 아이콘을 표시하고, 장치가 바뀌면 맞춰서 변경
 * - 아이콘은 Common Input Settings에 등록된 Controller Data에서 찾음
 *   (여러 키를 묶은 Key Set이 있으면 그 아이콘 하나, 없으면 키마다 아이콘)
 * - Input Action이 아니라 키를 직접 지정 (내비게이션처럼 Input Action이 없는 입력도 안내하기 위함)
 */
UCLASS(Abstract)
class CXVI_API UCXInputHintWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativePreConstruct() override;

	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

private:
	void HandleInputMethodChanged(ECommonInputType InInputType);

	void RefreshIcons(ECommonInputType InInputType);

	void AddIcon(const FSlateBrush& InBrush);

protected:
	// 설명 (WBP에 배치한 인스턴스 마다 따로 지정)
	UPROPERTY(EditAnywhere, Category = "Hint")
	FText Label;

	// 키보드 / 마우스 사용 시 표시할 키
	UPROPERTY(EditAnywhere, Category = "Hint")
	TArray<FKey> KeyboardKeys;

	// 게임패드 사용 중일 때 표시할 키
	UPROPERTY(EditAnywhere, Category = "Hint")
	TArray<FKey> GamepadKeys;

	// 아이콘 사이 간격
	UPROPERTY(EditAnywhere, Category = "Hint")
	FMargin IconPadding;
	
	// 아이콘 크기 (0이면 Controller Data의 브러시 크기 사용)
	UPROPERTY(EditAnywhere, Category = "Hint", meta = (ClampMin = "0"))
	FVector2D IconSize = FVector2D::ZeroVector;

	// 설명 폰트 크기 (0이면 WBP의 LabelText에 지정한 크기 사용)
	UPROPERTY(EditAnywhere, Category = "Hint", meta = (ClampMin = "0"))
	float FontSize = 0.f;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LabelText;

	// 아이콘 이미지를 생성해서 넣는 칸
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> IconBox;
};

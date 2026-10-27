// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"

#include "CXButtonBase.generated.h"

class UCommonTextBlock;

/**
 * 프로젝트 공통 버튼 베이스
 * - 포커스 상태가 즉 하이라이트 : 포커스를 받으면 Selected 스타일로 표시하고, 잃으면 해제
 *	(CommonButton은 포커스만으로는 스타일이 바뀌지 않아서 Selected 상태를 하이라이트로 사용)
 * - 마우스 Hover 시 포커스를 가져와서 하이라이트가 하나만 보이도록 유지
 * - 하이라이트 모양은 Style(CommonButtonStyle)의 Selected 항목에서 지정
 * - ButtonTextBlock이 있으면 ButtonText를 표시하고, 버튼 상태에 맞는 텍스트 스타일 적용
 */
UCLASS(Abstract)
class CXVI_API UCXButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()
	
public:
	UCXButtonBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativePreConstruct() override;

	virtual void HandleFocusLost() override;

	virtual void NativeOnHovered() override;

	virtual void NativeOnCurrentTextStyleChanged() override;

protected:
	// 버튼에 표시할 글자 (WBP에 배치한 인스턴스마다 지정)
	UPROPERTY(EditAnywhere, Category = "Button")
	FText ButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> ButtonTextBlock;
};

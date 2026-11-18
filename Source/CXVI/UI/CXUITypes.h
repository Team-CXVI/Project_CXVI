// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "CXUITypes.generated.h"

/**
 * UI 메시지 타입
 * - 다른 영역에서 UI 클래스를 몰라도 화면을 열 수 있도록 태그만 전달
 */

// Message.UI.OpenScreen 태그로 전달하는 메시지 구조체
// 태그에 연결된 위젯과 레이어는 UCXUISettings::Screens에서 지정
USTRUCT(BlueprintType)
struct CXVI_API FCXOpenScreenMessage
{
	GENERATED_BODY()

public:
	// 열 화면 (UI.Screen.*)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (Categories = "UI.Screen"))
	FGameplayTag ScreenTag;

	// 화면에 전달할 구분 값 (UI.ScreenContext.*). 필요 없는 화면은 비워 둠
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (Categories = "UI.ScreenContext"))
	FGameplayTag ContextTag;
};
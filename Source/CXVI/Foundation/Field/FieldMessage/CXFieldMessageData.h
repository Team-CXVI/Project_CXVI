// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Foundation/UserData/CXUserDataTypes.h"

#include "CXFieldMessageData.generated.h"

/**
 * 필드 GameplayMessage 구조체 모음
 * - 필드 액터가 방송하고 UI 등이 받는 메시지
 * - 메시지는 일어난 사실만 담음. 표시 여부 / 방식은 받는 쪽이 결정
 */

// Message.Field.ItemAcquired 태그로 전달하는 메시지 구조체
// 필드 아이템 하나를 주웠을 때 1회 방송 (보상이 여러 개면 한 메시지에 모두 담음)
USTRUCT(BlueprintType)
struct CXVI_API FCXItemAcquiredMessage
{
	GENERATED_BODY()

public:
	// 실제로 인벤토리에 추가된 아이템 목록 (ItemId = DT_Item Row 이름, Count = 습득 수량)
	UPROPERTY(BlueprintReadOnly, Category = "Field")
	TArray<FCXItemStack> Items;
};

// Message.Field.PartyHealed 태그로 전달하는 메시지 구조체
// 필드에서 파티 HP를 회복했을 때 1회 방송 (포션 사용, 깃발 휴식). 회복 사실만 알림
USTRUCT(BlueprintType)
struct CXVI_API FCXPartyHealedMessage
{
	GENERATED_BODY()
};

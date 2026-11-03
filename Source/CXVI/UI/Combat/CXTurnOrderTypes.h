// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "CXTurnOrderTypes.generated.h"

class ACXCombatCharacterBase;
class UTexture2D;

/**
 * 턴 순서 UI 한 칸의 표시 데이터
 * - UCXCombatViewModel이 GetUpcomingTurns() 순서대로 만들어서 UCXTurnOrderWidget에 전달
 * - 같은 캐릭터가 여러 칸에 나올 수 있음 (예측 턴)
 */
USTRUCT(BlueprintType)
struct CXVI_API FCXTurnOrderEntryData
{
	GENERATED_BODY()

public:
	// 이전 목록과 비교할 때 쓰는 식별 값
	UPROPERTY(Transient)
	TWeakObjectPtr<ACXCombatCharacterBase> Character;

	// 비어 있으면 초상화를 숨김
	UPROPERTY(Transient)
	TSoftObjectPtr<UTexture2D> Portrait;
};

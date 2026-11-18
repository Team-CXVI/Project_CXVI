#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"

#include "CXItemTypes.generated.h"

class UTexture2D;

/*
* 아이템 기본 정보 테이블 Row
* RowName = ItemId
*/
USTRUCT(BlueprintType)
struct CXVI_API FCXItemRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (Categories = "Item.Type"))
	FGameplayTag ItemType;

	// 0번은 강화 레벨 1. 회복량의 단위는 아이템 효과 종류에 따라 해석한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recovery")
	TArray<float> RecoveryValues;

	// {Recovery} 인자에 강화 레벨의 회복량 표시값이 들어감 (FormatDescription)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (MultiLine = "true"))
	FText Description;

public:
	// 강화 레벨(1부터)의 회복량 원본 값. 칸이 없거나 유한수가 아니면 false
	bool TryGetRecoveryValue(int32 InItemLevel, float& OutValue) const;

	// Description의 {Recovery}를 강화 레벨의 회복량 표시값으로 채운 설명
	// - HealingTint : 최대 HP 비율(0~1) → % 정수 (×100, 반올림)
	// - EnergyTint  : 고정 회복량 → 정수 (반올림)
	// - 그 외 타입 / 값 없음 : Description 원문
	FText FormatDescription(int32 InItemLevel) const;
};
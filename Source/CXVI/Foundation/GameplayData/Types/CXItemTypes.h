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
};
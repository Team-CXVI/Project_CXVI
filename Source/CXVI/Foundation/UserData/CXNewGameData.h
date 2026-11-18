// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CXUserDataTypes.h"

#include "CXNewGameData.generated.h"

/**
 * 새 게임 시작 시의 기본 파티 설정
 */
USTRUCT(BlueprintType)
struct CXVI_API FCXNewGameMember
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NewGame", meta = (AllowedTypes = "CXCharacterDefinition"))
	FPrimaryAssetId CharacterId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NewGame", meta = (ClampMin = 1))
	int32 StartLevel = 1;
};

/**
 * 새 게임 시작 데이터 (시작 파티, 시작 아이템)
 * - Project Settings > Game > CXVI Gameplay Data > NewGameData 에 지정
 * - 시작 레벨(맵)은 CXVI Level Transition의 NewGameLevel을 사용
 */
UCLASS(BlueprintType)
class CXVI_API UCXNewGameData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NewGame")
	TArray<FCXNewGameMember> PartyMembers;

	// ItemId = DT_Item의 Row 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NewGame")
	TArray<FCXItemStack> StartItems;
};

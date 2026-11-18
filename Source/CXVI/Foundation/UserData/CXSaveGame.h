// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CXUserDataTypes.h"

#include "CXSaveGame.generated.h"

/**
 * 저장 형식 = 헤더(고정 데이터) + 유저 데이터 스냅샷
 * - 저장 형식이 바뀌면 CurrentVersion을 올리고 로드 시 변환 처리
 */
UCLASS()
class CXVI_API UCXSaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	static constexpr int32 CurrentVersion = 1;

public:
	UPROPERTY()
	FCXSaveHeader Header;

	UPROPERTY()
	FCXUserData UserData;
};

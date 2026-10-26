// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CXPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API ACXPlayerController : public APlayerController
{
	GENERATED_BODY()
	
	
protected:
	// PlayerCameraManager가 생성된 직후, 레벨 전환 중이면 화면 가림 상태를 유지처리
	virtual void PostInitializeComponents() override;

	virtual void ReceivedPlayer() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
};

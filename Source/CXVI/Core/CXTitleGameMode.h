// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "CXTitleGameMode.generated.h"

class UCXActivatableWidget;

/**
 * 타이틀 레벨의 GameMode
 * - BeginPlay에서 타이틀 화면을 UI.Layer.Menu에 Push
 * - 타이틀은 Scene을 쓰지 않으므로 바로 레벨 전환에 준비 완료를 알림
 */
UCLASS()
class CXVI_API ACXTitleGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ACXTitleGameMode();

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UCXActivatableWidget> TitleScreenClass;
};

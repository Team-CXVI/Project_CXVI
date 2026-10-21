// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "CXSceneSuspendable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCXSceneSuspendable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Scene Suspend / Resume 알림을 받는 액터용 인터페이스
 * - Persistant Level 액터(플레이어 캐릭터)는 서브레벨 Hide 영향을 받지 않으므로 여기서 숨김/입력/이동을 직접 처리
 * - Sub Level 액터는 Hide 될 때 EndPlay(RemovedFromWorld), 다시 Show 될 때 BeginPlay가 재호출 됨
 *  -> BeginPlay에서 게임 상태(Hp 등을) 초기화 하면 안됨
 */
class CXVI_API ICXSceneSuspendable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, Category = "Scene")
	void OnSceneSuspended();

	UFUNCTION(BlueprintNativeEvent, Category = "Scene")
	void OnSceneResumed();
	
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "CXInteractableInterface.generated.h"

// 프로젝트 설정 > Collision > Object Channels 에서 첫 번째 커스텀 채널을 "Interactable"로 만들었다고 가정
#define INTERACTABLE_CHANNEL ECC_GameTraceChannel1

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCXInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 상호작용 가능한 기믹이 구현하는 인터페이스.
 * 기믹은 "판단 결과를 알려주고, 시키는 대로 동작"만 한다. 최종 판단과 타겟 선정은 송신기 책임.
 * BlueprintNativeEvent 이므로 호출은 반드시 ICXInteractableInterface::Execute_XXX(Obj, ...) 로 한다.
 */
class CXVI_API ICXInteractableInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	/** 이 기믹이 Interactor와 지금 상호작용 가능한가 (기믹 쪽 조건만 판단) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AActor* Interactor) const;

	/** 실제 기믹 활성화 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AActor* Interactor);

	/** 송신기가 시키는 대로 프롬프트 UI를 켜고 끄기만 한다. 여기서 조건 판단 금지. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void SetInteractionPromptVisible(bool bVisible);
	
};

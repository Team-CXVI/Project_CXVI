// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actor/Field/Interactions/CXInteractableGimmick.h"

#include "CXGrapplingHookGimmick.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API ACXGrapplingHookGimmick : public ACXInteractableGimmick
{
	GENERATED_BODY()
	
public:
	ACXGrapplingHookGimmick();

protected:
	virtual void Interact_Implementation(AActor* Interactor) override;

protected:
	// 레벨에 배치한 트리거의 Details에서 타겟/착지 액터를 지정한다.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Grappling")
	TObjectPtr<AActor> GrapplingTarget;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Grappling")
	TObjectPtr<AActor> LandingTarget;
};

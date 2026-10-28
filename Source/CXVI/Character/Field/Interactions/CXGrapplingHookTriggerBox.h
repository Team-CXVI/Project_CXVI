// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "CXGrapplingHookTriggerBox.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API ACXGrapplingHookTriggerBox : public ATriggerBox
{
	GENERATED_BODY()

public:
	ACXGrapplingHookTriggerBox(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;

protected:
	UFUNCTION()
	void HandleActorEndOverlap(
		AActor* OverlappedActor,
		AActor* OtherActor);

	UFUNCTION()
	void HandleActorBeginOverlap(
		AActor* OverlappedActor,
		AActor* OtherActor
	);

protected:
	// 레벨에 배치한 트리거의 Details에서 타겟/착지 액터를 지정한다.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Grappling")
	TObjectPtr<AActor> GrapplingTarget;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Grappling")
	TObjectPtr<AActor> LandingSurface;
	// 이 그래플링 기믹이 소유하는 UI
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grappling|UI")
	TObjectPtr<class UWidgetComponent> GrapplingUI;
};

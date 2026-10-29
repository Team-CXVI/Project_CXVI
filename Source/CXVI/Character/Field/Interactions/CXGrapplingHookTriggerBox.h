// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "CXGrapplingHookTriggerBox.generated.h"

/**
 * 
 */
class ACXFieldPlayerCharacter;
class ACharacter;
class UAbilitySystemComponent;
class UUserWidget;

UCLASS()
class CXVI_API ACXGrapplingHookTriggerBox : public ATriggerBox
{
	GENERATED_BODY()

public:
	ACXGrapplingHookTriggerBox(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
	void UpdateGrapplingUI(ACXFieldPlayerCharacter* Player, bool bCanInteract);
	void ObserveUIState(ACXFieldPlayerCharacter* Player);
	void StopObservingUIState();
	void HandleUIStateTagChanged(const FGameplayTag Tag, int32 NewCount);

	TWeakObjectPtr<ACXFieldPlayerCharacter> ObservedPlayer;
	TWeakObjectPtr<UAbilitySystemComponent> ObservedASC;
	FDelegateHandle AirborneTagHandle;
	FDelegateHandle AbilityUITagHandle;
	FDelegateHandle MovingBlockTagHandle;
	TWeakObjectPtr<UUserWidget> LastPromptWidget;
	bool bPromptShown = false;

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

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

	// UI 표시 조건. 그래플링 어빌리티도 같은 조건으로 사용 가능 여부를 확인한다.
	bool CanShowGrapplingUI(const ACXFieldPlayerCharacter* Player) const;

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
	void RefreshGrapplingUI();

	TWeakObjectPtr<ACXFieldPlayerCharacter> ObservedPlayer;
	TWeakObjectPtr<UAbilitySystemComponent> ObservedASC;
	FDelegateHandle AirborneTagHandle;
	FDelegateHandle AbilityUITagHandle;
	FDelegateHandle MovingBlockTagHandle;
	TWeakObjectPtr<UUserWidget> LastPromptWidget;
	bool bPromptShown = false;

	// 시선처럼 태그 이벤트가 없는 그래플링 고유 조건만 주기적으로 확인한다.
	FTimerHandle UIRefreshTimerHandle;

	// 플레이어 정면 기준으로 그래플링 타겟이 이 각도 안에 있을 때만 UI를 표시한다.
	UPROPERTY(EditAnywhere, Category = "Grappling|UI", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float PromptFacingAngle = 45.0f;

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

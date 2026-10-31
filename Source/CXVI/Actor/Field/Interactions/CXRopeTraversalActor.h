// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "CXRopeTraversalActor.generated.h"

class USceneComponent;
class UBoxComponent;
class UPrimitiveComponent;
class UWidgetComponent;
class UAbilitySystemComponent;
class UUserWidget;
class ACXFieldPlayerCharacter;

UENUM(BlueprintType)
enum class ECXRopeTraversalDirection :uint8
{
	None,
	Up,
	Down
};

UCLASS()
class CXVI_API ACXRopeTraversalActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACXRopeTraversalActor();

public:
	bool GetTraversalPoints(ECXRopeTraversalDirection Direction, FVector& OutEntry, FVector& OutDestination, FVector& OutExit) const;

	bool GetApproachPoint(ECXRopeTraversalDirection Direction, FVector& OutApproach)const;

	bool IsFacingRopePoint(const AActor* Player, ECXRopeTraversalDirection Direciton);

	bool CanShowRopeUI(const ACXFieldPlayerCharacter* Player);
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UFUNCTION()
	void HandleInteractionBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void HandleInteractionEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

protected:
	// bInRange가 false면 범위 이탈로 보고 위젯에 숨김을 요청하고, 범위 안의 숨김 조건은 여기서 즉시 숨긴다.
	void UpdateRopeUI(ACXFieldPlayerCharacter* Player, bool bInRange);
	
	void ObserveUIState(ACXFieldPlayerCharacter* Player);
	void StopObservingUIState();
	void HandleUIStateTagChanged(const FGameplayTag Tag, int32 NewCount);
	void RefreshRopeUI();

	TWeakObjectPtr<ACXFieldPlayerCharacter> ObservedPlayer;
	TWeakObjectPtr<UAbilitySystemComponent> ObservedASC;
	FDelegateHandle AirborneTagHandle;
	FDelegateHandle AbilityUITagHandle;
	FDelegateHandle MovingBlockTagHandle;
	TWeakObjectPtr<UUserWidget> LastPromptWidget;
	bool bPromptShown = false;

	// 시선처럼 태그 이벤트가 없는 로프 고유 조건만 주기적으로 확인한다.
	FTimerHandle UIRefreshTimerHandle;

	// 플레이어가 들어와 있는 쪽(하단/상단)의 UI.
	TWeakObjectPtr<UWidgetComponent> RopePromptUI;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope")
	TObjectPtr<USceneComponent> BottomRopePoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope")
	TObjectPtr<USceneComponent> TopRopePoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope")
	TObjectPtr<USceneComponent> BottomExitPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope")
	TObjectPtr<USceneComponent> TopExitPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope")
	TObjectPtr<UBoxComponent> BottomInteractionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope")
	TObjectPtr<UBoxComponent> TopInteractionBox;

	// 하단 진입 지점에 표시할 UI.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope|UI")
	TObjectPtr<UWidgetComponent> BottomPromptUI;

	// 상단 진입 지점에 표시할 UI.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope|UI")
	TObjectPtr<UWidgetComponent> TopPromptUI;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "CXExpeditionFlag.generated.h"

class ACXFieldPlayerCharacter;
class UAbilitySystemComponent;
class UUserWidget;

UCLASS()
class CXVI_API ACXExpeditionFlag : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACXExpeditionFlag();

public:
	// 이 플레이어에게 플래그를 사용할 수 있는지 물어보는 함수.
	bool CanFlag(const ACXFieldPlayerCharacter* Player)const;

	void CompleteFlag();

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
	void UpdateFlagUI(ACXFieldPlayerCharacter* Player, bool bInRange);
	bool CanShowFlagUI(const ACXFieldPlayerCharacter* Player) const;
	void ObserveUIState(ACXFieldPlayerCharacter* Player);
	void StopObservingUIState();
	void HandleUIStateTagChanged(const FGameplayTag Tag, int32 NewCount);
	void RefreshFlagUI();

	TWeakObjectPtr<ACXFieldPlayerCharacter> ObservedPlayer;
	TWeakObjectPtr<UAbilitySystemComponent> ObservedASC;
	FDelegateHandle AirborneTagHandle;
	FDelegateHandle AbilityUITagHandle;
	FDelegateHandle MovingBlockTagHandle;
	TWeakObjectPtr<UUserWidget> LastPromptWidget;
	bool bPromptShown = false;

	// 시선·몽타주처럼 태그 이벤트가 없는 깃발 고유 조건만 주기적으로 확인한다.
	FTimerHandle UIRefreshTimerHandle;

	// 플레이어 정면 기준으로 깃발이 이 각도 안에 있을 때만 UI를 표시한다.
	UPROPERTY(EditAnywhere, Category = "ExpeditionFlag|UI", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float PromptFacingAngle = 45.0f;



protected:

	// 원정대 깃발 메시.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExpeditionFlag")
	TObjectPtr<UStaticMeshComponent> ExpeditionFlag;

	// 상호작용 가능하게 할 트리거 스피어.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ExpeditionFlag")
	TObjectPtr<class USphereComponent> ExpeditionInteractionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ExpeditionFlag|UI")
	TObjectPtr<class UWidgetComponent> FlagPromptUI;
	
};

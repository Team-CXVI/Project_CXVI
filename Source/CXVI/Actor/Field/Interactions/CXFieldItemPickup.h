// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "CXVI/Foundation/UserData/CXUserDataTypes.h"
#include "CXFieldItemPickup.generated.h"

class ACXFieldPlayerCharacter;
class UAbilitySystemComponent;

UCLASS()
class CXVI_API ACXFieldItemPickup : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ACXFieldItemPickup();

public:
	// 스포너가 FinishSpawning 전에 호출.
	void InitializePickup(FName InSpawnId, const TArray<FCXItemStack>& InRewards);

	// 이 플레이어가 아이템을 주울 수 있는지 물어보는 함수.
	bool CanPickup(const ACXFieldPlayerCharacter* Player) const;

	// 보상 지급 -> 획득 기록 -> 제거.
	void Pickup(ACXFieldPlayerCharacter* Player);

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
	// bInRange가 false면 범위 이탈로 보고 숨기고, 범위 안이면 표시 조건을 확인한다.
	void UpdateItemUI(ACXFieldPlayerCharacter* Player, bool bInRange);
	bool CanShowItemUI(const ACXFieldPlayerCharacter* Player) const;
	void ObserveUIState(ACXFieldPlayerCharacter* Player);
	void StopObservingUIState();
	void HandleUIStateTagChanged(const FGameplayTag Tag, int32 NewCount);
	void RefreshItemUI();

	TWeakObjectPtr<ACXFieldPlayerCharacter> ObservedPlayer;
	TWeakObjectPtr<UAbilitySystemComponent> ObservedASC;
	FDelegateHandle AirborneTagHandle;
	FDelegateHandle AbilityUITagHandle;
	FDelegateHandle MovingBlockTagHandle;
	// 메뉴가 열린 동안 숨김.
	FDelegateHandle InMenuTagHandle;

	// 시선처럼 태그 이벤트가 없는 조건만 주기적으로 확인한다.
	FTimerHandle UIRefreshTimerHandle;

	// 플레이어 정면 기준으로 아이템이 이 각도 안에 있을 때만 UI를 표시한다.
	UPROPERTY(EditAnywhere, Category = "Item|UI", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float PromptFacingAngle = 45.0f;

protected:
	// 필드 아이템 메시.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	// 상호작용 가능하게 할 트리거 스피어.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<class USphereComponent> ItemPickupInteractionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|UI")
	TObjectPtr<class UWidgetComponent> ItemPickupPromptUI;

private:
	// 이 아이템을 생성한 스포너의 ID. 획득 기록에 사용.
	UPROPERTY(VisibleInstanceOnly, Category = "Item")
	FName SpawnId = NAME_None;

	// 획득 시 지급할 아이템.
	UPROPERTY(VisibleInstanceOnly, Category = "Item")
	TArray<FCXItemStack> Rewards;
};

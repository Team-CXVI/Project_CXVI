// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXVI/Foundation/UserData/CXUserDataTypes.h"
#include "CXFieldItemSpawner.generated.h"

class UArrowComponent;
class ACXFieldItemPickup;

/**
 * 필드 아이템 배치 지점
 * - 한 번 획득한 아이템은 다시 생성하지 않음 (새 게임에서만 초기화)
 * - 획득 기록은 UCXUserDataSubsystem의 CollectedItemSpawnIds에 SpawnId로 저장
 */
UCLASS()
class CXVI_API ACXFieldItemSpawner : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ACXFieldItemSpawner();

public:
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	ACXFieldItemPickup* SpawnPickup();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	// 생성 위치와 방향을 에디터에서 표시.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner")
	TObjectPtr<UArrowComponent> SpawnPoint;

	// 배치된 스포너마다 다르게 지정할 고유 ID. 게임 전체에서 겹치면 안 됨. (예: Lv01_Item_001)
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Spawner")
	FName SpawnId = NAME_None;

	// 획득 시 지급할 아이템. (ItemId = DT_Item의 Row 이름)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner")
	TArray<FCXItemStack> Rewards;

	// 생성할 필드 아이템 클래스.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner")
	TSubclassOf<ACXFieldItemPickup> PickupClass;

	// 현재 이 스포너가 생성한 아이템.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Spawner")
	TObjectPtr<ACXFieldItemPickup> SpawnedPickup;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXFieldEnemySpawner.generated.h"

class UArrowComponent;
class ACXFieldEnemy;
class USphereComponent;

UCLASS()
class CXVI_API ACXFieldEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ACXFieldEnemySpawner();

public:
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	ACXFieldEnemy* SpawnEnemy();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 테스트용 정찰 반경 표시
protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	// 에디터에서 정찰 반경을 표시하는 컴포넌트.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|Patrol")
	TObjectPtr<USphereComponent> PatrolRangePreview;

protected:
	// 생성 위치와 방향을 에디터에서 표시.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner")
	TObjectPtr<UArrowComponent> SpawnPoint;

	// 배치된 스포너마다 다르게 지정할 고유 ID.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Spawner")
	FName SpawnId = NAME_None;

	// 전투에서 사용할 적 편성을 식별.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Combat")
	FName EncounterId = NAME_None;

	// 생성할 몬스터 클래스.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner")
	TSubclassOf<ACXFieldEnemy> EnemyClass;

	// 이 스포너가 속한 휴식 그룹.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner")
	FName RestGroupId = NAME_None;

	// 휴식으로 재생성할 수 있는지 확인.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner")
	bool bRespawnOnRest = true;

	// 현재 이 몬스터가 생성한 몬스터.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Spawner")
	TObjectPtr<ACXFieldEnemy> SpawnedEnemy;

	// 이 지점에서 생성된 몬스터의 정찰 반경.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spanwer|Patrol")
	float PatrolRadius = 500.0f;
};
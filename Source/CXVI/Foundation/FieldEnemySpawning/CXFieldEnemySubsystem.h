// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CXFieldEnemySubsystem.generated.h"

// 몬스터는 없어져도 이 정보는 월드에 남는다.
USTRUCT()
struct FCXFieldEnemySpawnState
{
	GENERATED_BODY()

	// 어느 지역의 휴식으로 재생성되는지 구분.
	UPROPERTY()
	FName RestGroupId = NAME_None;

	// 플레이어가 처치했는지 여부.
	UPROPERTY()
	bool bDefeated = false;

	// 깃발 휴식으로 재생성할 수 있는 몬스터인지.
	UPROPERTY()
	bool bRespawnOnRest = true;

};

/**
 * 
 */

class ACXFieldEnemySpawner;

UCLASS()
class CXVI_API UCXFieldEnemySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	// 스폰 지점을 등록한다.
	// 이미 등록된 ID라면 기존 상태를 유지한다.
	UFUNCTION(BlueprintCallable, Category = "Field|Enemy")
	bool RegisterSpawnPoint(FName SpawnId, FName RestGroupId, bool bRespawnOnRest);

	// 등록된 적의 처치 여부를 조회한다.
	// 반환값: 조회 성공 여부.
	// bOutDefeated: 실제 처치 여부.
	UFUNCTION(BlueprintPure, Category ="Field|Enemy")
	bool TryGetEnemyDefeated(FName SpawnId, bool& bOutDefeated) const;

	// 등록된 적을 처치 상태로 기록한다.
	UFUNCTION(BlueprintCallable, Category = "Field|Enemy")
	bool MarkEnemyDefeated(FName SpawnId);

	// 스포너 섹션.
public:
	bool RegisterSpawner(FName SpawnId, ACXFieldEnemySpawner* Spawner);

	void UnRegisterSpawner(FName SpawnId, ACXFieldEnemySpawner* Spawner);

	UFUNCTION(BlueprintPure, Category = "Field|Enemy")
	ACXFieldEnemySpawner* FindSpawner(FName SpawnId) const;
	
	// 휴식 섹션.
public:
	// 휴식으로 복원할 적의 처치 기록을 초기화한다.
	// 반환값: 이번에 초기화된 SpawnId 목록.
	// RestGroupId가 None 이면 모든 그룹을 대상으로 본다.
	UFUNCTION(BlueprintCallable, Category = "Field|Enemy")
	TArray<FName> ResetDefeatedEnemies(FName RestGroupId = NAME_None);

private:
	// 스폰 지점의 고유 ID -> 해당 지점의 적 상태.
	UPROPERTY(Transient)
	TMap<FName, FCXFieldEnemySpawnState>SpawnStates;

	// 현재 등록한 스포너.
	UPROPERTY(Transient)
	TMap<FName, TWeakObjectPtr<ACXFieldEnemySpawner>> RegisteredSpawner;
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "CXFieldEnemySubsystem.h"
#include "CXFieldEnemySpawner.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldEnemySubsystem)
bool UCXFieldEnemySubsystem::RegisterSpawnPoint(FName SpawnId, FName RestGroupId, bool bRespawnOnRest)
{
	// 고유 ID가 없으면 등록할 수 없다.
	if (SpawnId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("ResisterSpawnPoint: SpawnId가 비어있습니다."));

		return false;
	}

	// 재등록 시 처치 기록을 덮어쓰지 않는다.
	if (SpawnStates.Contains(SpawnId))
	{
		return true;
	}

	// 처음 등록되는 지점의 초기 상태.
	FCXFieldEnemySpawnState NewState;
	NewState.RestGroupId = RestGroupId;
	NewState.bDefeated = false;
	NewState.bRespawnOnRest = bRespawnOnRest;

	SpawnStates.Add(SpawnId, NewState);

	return true;
}

bool UCXFieldEnemySubsystem::TryGetEnemyDefeated(FName SpawnId, bool& bOutDefeated) const
{
	// 조회 실패 시에도 출력값을 초기화한다.
	bOutDefeated = false;

	const FCXFieldEnemySpawnState* State = SpawnStates.Find(SpawnId);

	if (!State)
	{
		return false;
	}

	bOutDefeated = State->bDefeated;
	
	return true;
}

bool UCXFieldEnemySubsystem::MarkEnemyDefeated(FName SpawnId)
{
	FCXFieldEnemySpawnState* State = SpawnStates.Find(SpawnId);

	if (!State)
	{
		UE_LOG(LogTemp, Warning, TEXT("MarkEnemyDefeated: 등록되지 않은 Spawn입니다. [%s]"), *SpawnId.ToString());

		return false;
	}

	State->bDefeated = true;

	return true;
}

bool UCXFieldEnemySubsystem::RegisterSpawner(FName SpawnId, ACXFieldEnemySpawner* Spawner)
{
	if (SpawnId.IsNone() || !IsValid(Spawner))
	{
		return false;
	}

	// 다른월드의 스포너는 등록 안함.
	if (Spawner->GetWorld() != GetWorld())
	{
		return false;
	}

	// 상태 등록이 먼저 되어 있어야 함.
	if (!SpawnStates.Contains(SpawnId))
	{
		return false;
	}

	const TWeakObjectPtr<ACXFieldEnemySpawner>* Existing = RegisteredSpawner.Find(SpawnId);

	if (Existing && Existing->IsValid() && Existing->Get() != Spawner)
	{
		UE_LOG(LogTemp, Warning, TEXT("RegisterSpawn: SpawnId가 중복되었습니다. [%s]"),*SpawnId.ToString());

		return false;
	}

	RegisteredSpawner.Add(SpawnId, TWeakObjectPtr<ACXFieldEnemySpawner>(Spawner));

	return true;
}

void UCXFieldEnemySubsystem::UnRegisterSpawner(FName SpawnId, ACXFieldEnemySpawner* Spawner)
{
	if (!Spawner)
	{
		return;
	}

	const TWeakObjectPtr<ACXFieldEnemySpawner>* Existing = RegisteredSpawner.Find(SpawnId);

	// 등록된 객체와 해제를 요청한 객체가 같은지 확인.
	if (Existing && *Existing == TWeakObjectPtr<ACXFieldEnemySpawner>(Spawner))
	{
		RegisteredSpawner.Remove(SpawnId);
	}
}

ACXFieldEnemySpawner* UCXFieldEnemySubsystem::FindSpawner(FName SpawnId) const
{
	const TWeakObjectPtr<ACXFieldEnemySpawner>* Found = RegisteredSpawner.Find(SpawnId);

	return Found ? Found->Get() : nullptr;
}

TArray<FName> UCXFieldEnemySubsystem::ResetDefeatedEnemies(FName RestGroupId)
{
	TArray<FName> ResetSpawnIds;

	for (auto& Pair : SpawnStates)
	{
		FCXFieldEnemySpawnState& State = Pair.Value;

		// 처치되지 않았거나 휴식 재생성이 금지된 몬스터는 제외.
		if (!State.bDefeated || !State.bRespawnOnRest)
		{
			continue;
		}

		// 특정 그룹을 지정했다면 같은 그룹만 처리.
		if (!RestGroupId.IsNone() && State.RestGroupId != RestGroupId)
		{
			continue;
		}

		State.bDefeated = false;

		ResetSpawnIds.Add(Pair.Key);
	}
	return ResetSpawnIds;
}

int32 UCXFieldEnemySubsystem::RespawnDefeatedEnemies(FName RestGroupId)
{
	// 먼저 대상 기록을 초기화하고 ID 목록을 확보.
	const TArray<FName> ResetSpawnIds = ResetDefeatedEnemies(RestGroupId);

	int32 NumRespawned = 0;
	
	for (const FName& SpawnId : ResetSpawnIds)
	{
		ACXFieldEnemySpawner* Spawner = FindSpawner(SpawnId);
		// 로드가 안된 지역 등 현재 스포너가 없는 경우.
		if (!IsValid(Spawner))
		{
			continue;
		}
		
		if (Spawner->SpawnEnemy() != nullptr)
		{
			++NumRespawned;
		}
		else
		{
			// 활성 지점에서 생성에 실패하면 다음 휴식 때 다시 시도할 수 있도록 복원.
			MarkEnemyDefeated(SpawnId);
		}
	}
	return NumRespawned;
}


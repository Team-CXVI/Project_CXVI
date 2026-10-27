// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXVI/Foundation/Combat/CXCombatTypes.h"

#include "CXCombatArena.generated.h"

class UArrowComponent;
class UCameraComponent;
class ACXCombatCharacterBase;

/**
 * 전투 아레나 (전투 서브레벨에 1개 배치)
 * - 슬롯(플레이어 / 적 위치), 전투 카메라
 * - 슬롯 위치에 전투원 생성 / 정리 (기존 UCXArenaManager 역할)
 * - 서브레벨이 Hide / Show 될 때 BeginPlay가 다시 호출되므로 BeginPlay에서 전투 상태를 초기화하지 않음
 */
UCLASS()
class CXVI_API ACXCombatArena : public AActor
{
	GENERATED_BODY()

public:
	ACXCombatArena();

	// 슬롯 위치에 전투원 생성 (이 액터와 같은 레벨). 실패하면 생성한 전투원을 정리하고 false
	bool SpawnCombatants(const FCXCombatEnterParams& InParams);

	// 생성한 전투원 Destroy
	void CleanupCombatants();

	FORCEINLINE const TArray<TObjectPtr<ACXCombatCharacterBase>>& GetPlayers() const { return PlayerCharacters; }
	FORCEINLINE const TArray<TObjectPtr<ACXCombatCharacterBase>>& GetEnemies() const { return EnemyCharacters; }

	FORCEINLINE int32 GetPlayerSlotCount() const { return PlayerSlots.Num(); }
	FORCEINLINE int32 GetEnemySlotCount() const { return EnemySlots.Num(); }

	FTransform GetPlayerSlotFromIndex(int32 InIndex) const;
	FTransform GetEnemySlotFromIndex(int32 InIndex) const;

private:
	bool SpawnParty(const TArray<TSubclassOf<ACXCombatCharacterBase>>& InClasses, bool bInPlayerParty);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slots")
	TArray<TObjectPtr<UArrowComponent>> PlayerSlots;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slots")
	TArray<TObjectPtr<UArrowComponent>> EnemySlots;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<UCameraComponent> BattleCamera;

private:
	// 생성한 전투원 (레벨에 저장되지 않도록 Transient)
	UPROPERTY(Transient)
	TArray<TObjectPtr<ACXCombatCharacterBase>> PlayerCharacters;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACXCombatCharacterBase>> EnemyCharacters;

	int32 MaxSlot = 3;
};

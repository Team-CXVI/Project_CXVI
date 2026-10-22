// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Foundation/Combat/CXArenaManager.h"

#include "CXVI/Actor/CXTestCombatArena.h"
#include "CXVI/Character/Combat/CXCombatCharacterBase.h"

#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXArenaManager)

UWorld* UCXArenaManager::GetWorld() const
{
    if (HasAnyFlags(RF_ClassDefaultObject) || !GetOuter())
    {
        return nullptr;
    }

    return GetOuter()->GetWorld();
}

bool UCXArenaManager::PrepareArena(const FCXArenaSetupData& SetupData, ACXTestCombatArena* InArena)
{
    if (!GetWorld() || !IsValid(InArena))
    {
        return false;
    }

    // 준비 중이거나 이미 전투원이 존재하면 중복 생성하지 않음.
    if (Arena.IsValid() || !PlayerCharacters.IsEmpty() ||
        !EnemyCharacters.IsEmpty())
    {
        return false;
    }

    if (SetupData.PlayerClasses.IsEmpty() ||
        SetupData.EnemyClasses.IsEmpty())
    {
        return false;
    }

    // 슬롯보다 파티 인원이 많으면 시작하지 않음.
    if (SetupData.PlayerClasses.Num() > InArena->GetPlayerSlotCount() ||
        SetupData.EnemyClasses.Num() > InArena->GetEnemySlotCount())
    {
        return false;
    }

    Arena = InArena;

    if (!SpawnParty(SetupData.PlayerClasses, true) ||
        !SpawnParty(SetupData.EnemyClasses, false))
    {
        CleanupArena();
        return false;
    }

    // 양쪽 생성이 모두 끝난 후에만 알림.
    OnArenaReady.Broadcast(PlayerCharacters, EnemyCharacters);
    return true;
}

void UCXArenaManager::CleanupArena()
{
    for (ACXCombatCharacterBase* Character : PlayerCharacters)
    {
        if (IsValid(Character))
        {
            Character->Destroy();
        }
    }

    for (ACXCombatCharacterBase* Character : EnemyCharacters)
    {
        if (IsValid(Character))
        {
            Character->Destroy();
        }
    }

    PlayerCharacters.Reset();
    EnemyCharacters.Reset();
    Arena.Reset();
    Arena = nullptr;
}

bool UCXArenaManager::SpawnParty(const TArray<TSubclassOf<ACXCombatCharacterBase>>& Classes, bool bPlayerParty)
{
    UWorld* World = GetWorld();

    if (!World || !Arena.IsValid())
    {
        return false;
    }

    TArray<TObjectPtr<ACXCombatCharacterBase>>& Characters =
        bPlayerParty ? PlayerCharacters : EnemyCharacters;

    for (int32 Index = 0; Index < Classes.Num(); ++Index)
    {
        const TSubclassOf<ACXCombatCharacterBase> CharacterClass =
            Classes[Index];

        if (!CharacterClass ||
            CharacterClass->HasAnyClassFlags(CLASS_Abstract))
        {
            return false;
        }

        FTransform SpawnTransform = bPlayerParty
            ? Arena->GetPlayerSlotFromIndex(Index)
            : Arena->GetEnemySlotFromIndex(Index);

        // 현재 프로젝트의 기존 높이 보정 유지.
        // 추후 캐릭터별 캡슐 크기를 기준으로 변경.
        SpawnTransform.AddToTranslation(FVector(0.f, 0.f, 100.f));

        FActorSpawnParameters SpawnParams;

        // 전투원이 아레나와 같은 서브레벨에 속하도록 지정.
        SpawnParams.OverrideLevel = Arena->GetLevel();
        SpawnParams.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        ACXCombatCharacterBase* Character =
            World->SpawnActor<ACXCombatCharacterBase>(
                CharacterClass,
                SpawnTransform,
                SpawnParams
            );

        if (!IsValid(Character))
        {
            return false;
        }

        Character->SetOriginTransform(Character->GetActorTransform());
        Characters.Add(Character);
    }

    return true;
}

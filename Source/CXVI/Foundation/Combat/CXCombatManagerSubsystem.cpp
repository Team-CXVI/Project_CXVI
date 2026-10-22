// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Foundation/Combat/CXCombatManagerSubsystem.h"

#include "CXVI/Character/Combat/CXCombatCharacterPlayer.h"
#include "CXVI/Character/Combat/CXCombatCharacterEnemy.h"
#include "CXVI/Character/Combat/CXCombatCharacterBase.h"
#include "CXVI/Actor/TestCombatArena.h"

#include "CXVI/Foundation/Combat/CXTurnManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatManagerSubsystem)

UCXCombatManagerSubsystem::UCXCombatManagerSubsystem()
{
	
}

void UCXCombatManagerSubsystem::RequestStartBattle(TArray<TSubclassOf<class ACXCombatCharacterBase>> Players, TArray<TSubclassOf<class ACXCombatCharacterBase>> Enemies, APlayerController* PlayerController)
{
	//UTurnManager* Manager = GetWorld()->GetSubsystem<UTurnManager>();

	if (!TurnManager || !IsValid(TestCombatArena) || !IsValid(PlayerController))
	{
		return;
	}

	// 기존 필드 캐릭터 저장
	APawn* FieldPawn = PlayerController->GetPawn();

	StoredFieldPawn = FieldPawn;
	StoredPlayerController = PlayerController;

	TArray<TObjectPtr<ACXCombatCharacterBase>> PlayerArray;
	TArray<TObjectPtr<ACXCombatCharacterBase>> EnemyArray;

	// 객체 생성
	for (int32 Index = 0; Index < Players.Num(); ++Index)
	{
		FTransform OriginTransform = TestCombatArena->GetPlayerSlotFromIndex(Index);

		// @TODO: 재원 현재 플레이어의 크기의 반만큼 올려서 조정중 나중에 플레이어 크기가 아니고 다른 큰 몬스터들이라면 개인의 크기값을 알고있어야할듯
		FVector OriginLocation = OriginTransform.GetLocation();
		OriginTransform.SetLocation(FVector(OriginLocation.X, OriginLocation.Y, OriginLocation.Z + 100.f));
		ACXCombatCharacterPlayer* NewPlayer = GetWorld()->SpawnActor<ACXCombatCharacterPlayer>(Players[Index], OriginTransform);
		NewPlayer->SetOriginTransform(OriginTransform);
		PlayerArray.Add(NewPlayer);
	}
	for (int32 Index = 0; Index < Enemies.Num(); ++Index)
	{
		FTransform OriginTransform = TestCombatArena->GetEnemySlotFromIndex(Index);
		FVector OriginLocation = OriginTransform.GetLocation();
		OriginTransform.SetLocation(FVector(OriginLocation.X, OriginLocation.Y, OriginLocation.Z + 100.f));
		ACXCombatCharacterEnemy* NewEnemy = GetWorld()->SpawnActor<ACXCombatCharacterEnemy>(Enemies[Index], OriginTransform);
		NewEnemy->SetOriginTransform(OriginTransform);
		EnemyArray.Add(NewEnemy);
	}

	if (PlayerArray.IsEmpty() || EnemyArray.IsEmpty())
	{
		return;
	}

	if (!IsValid(PlayerArray[0]))
	{
		return;
	}

	//AGASPlayerController* GASController = Cast<AGASPlayerController>(PlayerController);

	//if (!IsValid(GASController))
	//{
	//	return;
	//}

	//for (int32 Index = 0; Index < PlayerArray.Num(); ++Index)
	//{

	//	GASController->AddCommandCharacter(Cast<ABattleCharacterPlayer>(PlayerArray[Index]));
	//}

	////GASController->Possess(PlayerArray[0]);
	//GASController->SetBattleInput(true);
	//GASController->SetViewTargetWithBlend(TestCombatArena, 0.f);

	//Manager->StartBattle(PlayerArray, EnemyArray);
}

void UCXCombatManagerSubsystem::RequestEndBattle()
{
	if (!IsValid(StoredFieldPawn) || !IsValid(StoredPlayerController))
	{
		return;
	}

	StoredPlayerController->Possess(StoredFieldPawn);

	/*if (AGASPlayerController* GASController = Cast<AGASPlayerController>(StoredPlayerController))
	{
		GASController->SetBattleInput(false);
	}

	StoredPlayerController->SetViewTargetWithBlend(StoredFieldPawn, 0.3f);

	StoredPlayerController = nullptr;
	StoredFieldPawn = nullptr;*/
}

void UCXCombatManagerSubsystem::PostInitialize()
{
	Super::PostInitialize();

	TurnManager = NewObject<UCXTurnManager>(this);
}

void UCXCombatManagerSubsystem::HandlePlayerInputModeChanged(bool bAllowed)
{
	/*AGASPlayerController* Controller = Cast<AGASPlayerController>(StoredPlayerController);

	if (!IsValid(Controller))
	{
		return;
	}

	Controller->SetBattleInputMode(bAllowed ? EBattleInputMode::Command : EBattleInputMode::Disabled);*/
}

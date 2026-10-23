// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/Combat/CXTestCombatArena.h"

#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXTestCombatArena)

// Sets default values
ACXTestCombatArena::ACXTestCombatArena()
{
	BattleArena = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Field"));
	BattleArena->SetupAttachment(RootComponent);

	BattleCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("BattleCamera"));
	BattleCamera->SetupAttachment(BattleArena);
	BattleCamera->SetRelativeLocationAndRotation(FVector(-850.f, 0.f, 400.f), FRotator(-30.f, 0.f, 0.f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ArenaMeshRef(
		TEXT("/Game/Combat/Environment/Combat/SM_BattleArena.SM_BattleArena")
	);

	if (ArenaMeshRef.Succeeded())
	{
		BattleArena->SetStaticMesh(ArenaMeshRef.Object);
	}

	for (int32 Index = 0; Index < MaxSlot; ++Index)
	{
		FName SlotName(*FString::Printf(TEXT("PlayerSlot%d"), Index));

		UArrowComponent* Slot = CreateDefaultSubobject<UArrowComponent>(SlotName);

		Slot->SetupAttachment(BattleArena);
		Slot->SetRelativeLocation(FVector(-300.f, -200.f + 200.f * Index, 0.f));
		PlayerSlots.Add(Slot);
	}

	for (int32 Index = 0; Index < MaxSlot; ++Index)
	{
		FName SlotName(*FString::Printf(TEXT("EnemySlot%d"), Index));

		UArrowComponent* Slot = CreateDefaultSubobject<UArrowComponent>(SlotName);

		Slot->SetupAttachment(BattleArena);
		Slot->SetRelativeLocation(FVector(300.f, -200.f + 200.f * Index, 0.f));
		Slot->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
		EnemySlots.Add(Slot);
	}
}

const FTransform ACXTestCombatArena::GetPlayerSlotFromIndex(int32 InIndex) const
{
	if (PlayerSlots.IsValidIndex(InIndex))
	{
		return PlayerSlots[InIndex]->GetComponentTransform();
	}

	return FTransform();
}

const FTransform ACXTestCombatArena::GetEnemySlotFromIndex(int32 InIndex) const
{
	if (EnemySlots.IsValidIndex(InIndex))
	{
		return EnemySlots[InIndex]->GetComponentTransform();
	}
	return FTransform();
}


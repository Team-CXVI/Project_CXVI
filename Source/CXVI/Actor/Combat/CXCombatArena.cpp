// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/Combat/CXCombatArena.h"

#include "CXVI/Character/Combat/CXCombatCharacterBase.h"
#include "Character/Combat/CXCombatCharacterEnemy.h"
#include "Character/Combat/CXCombatCharacterPlayer.h"
#include "Actor/Combat/CXCombatMenuCameraData.h"
#include "UI/Combat/CXCombatCommandMenuComponent.h"
#include "UI/Combat/CXDamageNumberComponent.h"

#include "Components/SceneComponent.h"
#include "Components/ArrowComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"

//TODO : Log 정의 따로 뺀 후에 교체(LogCXCombat 정의 때문에 들어가 있음)
#include "CXVI/Foundation/Combat/CXCombatManagerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatArena)

ACXCombatArena::ACXCombatArena()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComp"));

	BattleCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("BattleCamera"));
	BattleCamera->SetupAttachment(RootComponent);
	BattleCamera->SetRelativeLocationAndRotation(FVector(-850.f, 0.f, 400.f), FRotator(-30.f, 0.f, 0.f));

	CommandMenu = CreateDefaultSubobject<UCXCombatCommandMenuComponent>(TEXT("CommandMenu"));
	CommandMenu->SetupAttachment(RootComponent);

	DamageNumber = CreateDefaultSubobject<UCXDamageNumberComponent>(TEXT("DamageNumber"));

	for (int32 Index = 0; Index < MaxSlot; ++Index)
	{
		FName SlotName(*FString::Printf(TEXT("PlayerSlot%d"), Index));

		UArrowComponent* Slot = CreateDefaultSubobject<UArrowComponent>(SlotName);

		Slot->SetupAttachment(RootComponent);
		Slot->SetRelativeLocation(FVector(-300.f, -200.f + 200.f * Index, 0.f));
		PlayerSlots.Add(Slot);
	}

	for (int32 Index = 0; Index < MaxSlot; ++Index)
	{
		FName SlotName(*FString::Printf(TEXT("EnemySlot%d"), Index));

		UArrowComponent* Slot = CreateDefaultSubobject<UArrowComponent>(SlotName);

		Slot->SetupAttachment(RootComponent);
		Slot->SetRelativeLocation(FVector(300.f, -200.f + 200.f * Index, 0.f));
		Slot->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
		EnemySlots.Add(Slot);
	}
}

bool ACXCombatArena::SpawnCombatants(const FCXCombatEnterParams& InParams)
{
	// 이미 전투원이 있으면 중복 생성하지 않음
	if (!PlayerCharacters.IsEmpty() || !EnemyCharacters.IsEmpty())
	{
		UE_LOG(LogCXCombat, Warning, TEXT("Arena : 이미 생성된 전투원이 있습니다."));
		return false;
	}

	if (InParams.PlayerClasses.IsEmpty() || InParams.EnemySpawns.IsEmpty())
	{
		UE_LOG(LogCXCombat, Warning, TEXT("Arena : 파티 정보가 비어 있습니다."));
		return false;
	}

	// 슬롯보다 파티 인원이 많으면 시작하지 않음
	if (InParams.PlayerClasses.Num() > GetPlayerSlotCount() ||
		InParams.EnemySpawns.Num() > GetEnemySlotCount())
	{
		UE_LOG(LogCXCombat, Warning, TEXT("Arena : 파티 인원이 슬롯 수보다 많습니다."));
		return false;
	}
	

	if (!SpawnParty(InParams.PlayerClasses, true) ||
		!SpawnParty(InParams.EnemySpawns, false))
	{
		UE_LOG(LogCXCombat, Warning, TEXT("Arena : 전투원 생성 실패"));
		CleanupCombatants();
		return false;
	}

	return true;
}

void ACXCombatArena::CleanupCombatants()
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
}

FTransform ACXCombatArena::GetPlayerSlotFromIndex(int32 InIndex) const
{
	if (PlayerSlots.IsValidIndex(InIndex))
	{
		return PlayerSlots[InIndex]->GetComponentTransform();
	}

	return FTransform();
}

FTransform ACXCombatArena::GetEnemySlotFromIndex(int32 InIndex) const
{
	if (EnemySlots.IsValidIndex(InIndex))
	{
		return EnemySlots[InIndex]->GetComponentTransform();
	}

	return FTransform();
}

void ACXCombatArena::ShowMenuCamera(ECommandMenuState MenuState, const ACXCombatCharacterBase* CameraSubject)
{
	if (IsValid(MenuCameraData.Get()))
	{
		if (const FName* PresetId = MenuCameraData->MenuCameraIds.Find(MenuState))
		{
			ShowCameraPreset(*PresetId, CameraSubject);
		}
	}
}

void ACXCombatArena::ShowCameraPreset(FName PresetId, const ACXCombatCharacterBase* CameraSubject, TOptional<FVector> AnchorLocationOverride)
{
	if (PresetId.IsNone() || !IsValid(MenuCameraData.Get()) || !IsValid(BattleCamera.Get()))
	{
		return;
	}

	const FCXMenuCameraPreset* Preset = MenuCameraData->CameraPresets.Find(PresetId);

	if (!Preset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[전투 카메라] 프리셋 '%s'를 찾지 못했습니다."),
			*PresetId.ToString());
		return;
	}

	const bool bHasCameraSubject = IsValid(CameraSubject);
	const FTransform CurrentTransform = bHasCameraSubject
		? CameraSubject->GetActorTransform()
		: GetActorTransform();
	FTransform CameraAnchor = CurrentTransform;

	if (AnchorLocationOverride.IsSet())
	{
		CameraAnchor.SetLocation(AnchorLocationOverride.GetValue());
	}
	else if (bHasCameraSubject && Preset->bUseOriginLocation)
	{
		const FTransform& Origin = CameraSubject->GetOriginTransform();
		FVector HomeLocation = Origin.GetLocation();

		// 현재 아레나의 복귀 보정과 같은 방식으로 XY만 원래 자리 사용.
		HomeLocation.Z = CurrentTransform.GetLocation().Z;

		CameraAnchor.SetLocation(HomeLocation);
		CameraAnchor.SetRotation(Origin.GetRotation());
	}

	const FVector SubjectLocation = CameraAnchor.GetLocation();
	FVector FocusLocation = GetActorLocation();


	if (!Preset->FocusName.IsNone())
	{
		USceneComponent* FocusComponent;
		if(bHasCameraSubject)
		{ 
			FocusComponent = CameraSubject->FindComponentByTag<USceneComponent>(Preset->FocusName);
		}
		else
		{
			FocusComponent = FindComponentByTag<USceneComponent>(Preset->FocusName);
		}

		if (!IsValid(FocusComponent))
		{
			UE_LOG(LogTemp, Warning, TEXT("[전투 카메라] %s에서 시선점 태그 '%s'를 찾지 못했습니다."),
				*GetNameSafe(CameraSubject), *Preset->FocusName.ToString());
			return;
		}
		const FVector LocalFocus = CurrentTransform.InverseTransformPosition(
			FocusComponent->GetComponentLocation());

		FocusLocation = CameraAnchor.TransformPosition(LocalFocus);
	}
	

	const FTransform CameraOffsetBasis = bHasCameraSubject
		? CameraSubject->GetOriginTransform()
		: GetActorTransform();

	const FVector WorldOffset =
		CameraOffsetBasis.TransformVectorNoScale(Preset->CameraOffset);

	const FVector TargetLocation = SubjectLocation + WorldOffset;
	const FVector LookDirection = FocusLocation - TargetLocation;

	if (LookDirection.IsNearlyZero())
	{
		return;
	}

	const FQuat TargetRotation = LookDirection.Rotation().Quaternion();

	CameraBlendDuration = Preset->BlendDuration;

	if (CameraBlendDuration <= 0.f)
	{
		bIsCameraBlending = false;
		BattleCamera->SetWorldLocationAndRotation(TargetLocation, TargetRotation);
		return;
	}

	// 이동 중 구도가 바뀌면 현재 카메라 위치와 회전에서 이어간다.
	CameraBlendStartLocation = BattleCamera->GetComponentLocation();
	CameraBlendStartRotation = BattleCamera->GetComponentQuat();
	CameraBlendTargetLocation = TargetLocation;
	CameraBlendTargetRotation = TargetRotation;
	CameraBlendElapsed = 0.f;
	
	bIsCameraBlending = true;
	SetActorTickEnabled(true);
}

void ACXCombatArena::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (!bIsCameraBlending || !IsValid(BattleCamera.Get()))
	{
		return;
	}

	CameraBlendElapsed += DeltaSeconds;

	const float Alpha = FMath::Clamp(CameraBlendElapsed / CameraBlendDuration, 0.f, 1.f);
	const float Blend = FMath::SmoothStep(0.f, 1.f, Alpha);



	BattleCamera->SetWorldLocationAndRotation(
		FMath::Lerp(CameraBlendStartLocation, CameraBlendTargetLocation, Blend),
		FQuat::Slerp(CameraBlendStartRotation, CameraBlendTargetRotation, Blend));

	if (Alpha >= 1.f)
	{
		bIsCameraBlending = false;
	}
}

bool ACXCombatArena::SpawnParty(const TArray<FCXCombatSpawnEntry>& InEntries, bool bInPlayerParty)
{
	UWorld* World = GetWorld();
	if (nullptr == World)
	{
		return false;
	}

	TArray<TObjectPtr<ACXCombatCharacterBase>>& Characters = bInPlayerParty ? PlayerCharacters : EnemyCharacters;

	for (int32 Index = 0; Index < InEntries.Num(); ++Index)
	{
		const FCXCombatSpawnEntry& Entry = InEntries[Index];

		// Scene 전환의 LoadAssets 단계에서 이미 로드됨. 로드 실패 시 nullptr → 아래 검사에서 실패 처리
		const TSubclassOf<ACXCombatCharacterBase> CharacterClass = Entry.CharacterClass.Get();

		//const TSubclassOf<ACXCombatCharacterBase> CharacterClass = InEntries[Index];
		if (!CharacterClass || CharacterClass->HasAnyClassFlags(CLASS_Abstract))
		{
			return false;
		}

		FTransform SpawnTransform = bInPlayerParty ? GetPlayerSlotFromIndex(Index) : GetEnemySlotFromIndex(Index);

		// 기존 높이 보정 유지. 추후 캐릭터별 캡슐 크기를 기준으로 변경
		SpawnTransform.AddToTranslation(FVector(0.f, 0.f, 100.f));

		FActorSpawnParameters SpawnParams;
		// 전투원이 아레나와 같은 서브레벨에 속하도록 지정
		SpawnParams.OverrideLevel = GetLevel();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (Entry.bHasInitialAttributes)
		{
			const UClass* ExpectedClass = bInPlayerParty ? ACXCombatCharacterPlayer::StaticClass() : ACXCombatCharacterEnemy::StaticClass();

			if (!CharacterClass->IsChildOf(ExpectedClass))
			{
				return false;
			}

			SpawnParams.CustomPreSpawnInitialization =
				[bInPlayerParty, EnemySkills = Entry.EnemySkills, Stats = Entry.InitialAttributes, SkillPrefix = Entry.SkillRowPrefix, bIsBoss = Entry.bIsBoss, DisplayName = Entry.DisplayName, Portrait = Entry.Portrait](AActor* Actor)
				{
					// 양 팀 공통. UI 초상화 (턴 순서 / 파티 패널)
					CastChecked<ACXCombatCharacterBase>(Actor)->SetPortrait(Portrait);

					if (bInPlayerParty)
					{
						auto* Player = CastChecked<ACXCombatCharacterPlayer>(Actor);
						Player->SetInitialAttributes(Stats);
						Player->SetSkillRowPrefix(SkillPrefix);
					}
					else
					{
						auto* Enemy = CastChecked<ACXCombatCharacterEnemy>(Actor);
						Enemy->SetInitialAttributes(Stats);
						Enemy->SetIsBoss(bIsBoss);
						Enemy->SetEnemyDisplayName(DisplayName);
						Enemy->SetInitialSkills(EnemySkills);
					}
				};
		}

		ACXCombatCharacterBase* Character = World->SpawnActor<ACXCombatCharacterBase>(CharacterClass, SpawnTransform, SpawnParams);
		if (!IsValid(Character))
		{
			return false;
		}

		Character->SetOriginTransform(Character->GetActorTransform());
		Characters.Add(Character);
	}

	return true;
}

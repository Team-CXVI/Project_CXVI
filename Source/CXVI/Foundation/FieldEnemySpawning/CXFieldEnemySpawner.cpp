// Fill out your copyright notice in the Description page of Project Settings.


#include "CXFieldEnemySpawner.h"
#include "CXFieldEnemySubsystem.h"
#include "CXVI/Character/Field/CXFieldEnemy.h"

#include "Engine/World.h"
#include "Components/ArrowComponent.h"
#include "Components/SphereComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldEnemySpawner)

// Sets default values
ACXFieldEnemySpawner::ACXFieldEnemySpawner()
{
	SpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnPoint"));
	SetRootComponent(SpawnPoint);

	PatrolRangePreview =
		CreateDefaultSubobject<USphereComponent>(
			TEXT("PatrolRangePreview")
		);

	PatrolRangePreview->SetupAttachment(SpawnPoint);

	// 반경은 월드 cm 기준이므로 부모 스케일의 영향을 받지 않음.
	PatrolRangePreview->SetAbsolute(false, false, true);

	// 표시 용도이므로 충돌·오버랩·내비게이션 영향 제거.
	PatrolRangePreview->SetCollisionEnabled(
		ECollisionEnabled::NoCollision
	);
	PatrolRangePreview->SetGenerateOverlapEvents(false);
	PatrolRangePreview->SetCanEverAffectNavigation(false);

	// 에디터에서 선택했을 때 표시하고, 게임 중에는 숨김.
	PatrolRangePreview->bDrawOnlyIfSelected = true;
	PatrolRangePreview->SetHiddenInGame(true);

	PatrolRangePreview->ShapeColor = FColor::Green;
	PatrolRangePreview->SetLineThickness(2.0f);

	PatrolRangePreview->SetSphereRadius(PatrolRadius, false);
}

ACXFieldEnemy* ACXFieldEnemySpawner::SpawnEnemy()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	UCXFieldEnemySubsystem* EnemySubsystem = World->GetSubsystem<UCXFieldEnemySubsystem>();
	if (!IsValid(EnemySubsystem))
	{
		return nullptr;
	}

	// 등록 여부와 처치 여부를 확인.
	bool bDefeated = false;

	const bool bFound = EnemySubsystem->TryGetEnemyDefeated(SpawnId, bDefeated);

	if (!bFound || bDefeated)
	{
		return nullptr;
	}

	// 같은 스포너에서 중복 생성하지 않음.
	if (IsValid(SpawnedEnemy))
	{
		return SpawnedEnemy.Get();
	}

	if (!IsValid(EnemyClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: EnemyClass가 지정되지 않았습니다."), *GetName());


		return nullptr;
	}

	FActorSpawnParameters SpawnParams;

	SpawnParams.Owner = this;

	// 몬스터를 스포너와 같은 레벨에 생성.
	SpawnParams.OverrideLevel = GetLevel();

	// 지점 위치에 충돌이 있으면 생성하지 않음.
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;


	// 생성 완료를 미루고 먼저 설정을 전달하기 위함.
	SpawnParams.bDeferConstruction = true;

	const FTransform SpawnTransform = SpawnPoint->GetComponentTransform();

	SpawnedEnemy = World->SpawnActor<ACXFieldEnemy>(EnemyClass, SpawnPoint->GetComponentTransform(), SpawnParams);

	if (!IsValid(SpawnedEnemy))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 몬스터 생성에 실패했습니다. 생성 위치의 충돌을 확인하세요."), *GetName());

		return nullptr;
	}

	// 생성 완료 전에 설정 전달.
	SpawnedEnemy->InitializePatrolRadius(PatrolRadius);
	SpawnedEnemy->InitializeSpawnId(SpawnId);
	

	SpawnedEnemy->FinishSpawning(SpawnTransform);

	if (!IsValid(SpawnedEnemy))
	{
		return nullptr;
	}

	return SpawnedEnemy.Get();
}

// Called when the game starts or when spawned
void ACXFieldEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	UCXFieldEnemySubsystem* EnemySubsystem = GetWorld()->GetSubsystem<UCXFieldEnemySubsystem>();

	if (!EnemySubsystem)
	{
		return;
	}

	const bool bRegistered = EnemySubsystem->RegisterSpawnPoint(SpawnId, RestGroupId, bRespawnOnRest);

	if (!bRegistered)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 스폰 지점 등록에 실패했습니다."), *GetName());

		return;
	}

	if (!EnemySubsystem->RegisterSpawner(SpawnId, this))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 스포너 참조 등록에 실패했습니다."), *GetName());

		return;
	}


	// 등록된 상태를 확인하고 몬스터 생성.
	SpawnEnemy();

}

void ACXFieldEnemySpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UWorld* World = GetWorld();

	if (World)
	{
		UCXFieldEnemySubsystem* EnemySubsystem = World->GetSubsystem<UCXFieldEnemySubsystem>();

		if (IsValid(EnemySubsystem))
		{
			EnemySubsystem->UnRegisterSpawner(SpawnId, this);
		}
	}

	// 스포너가 종료되면 담당 몬스터도 정리.
	if (IsValid(SpawnedEnemy))
	{
		SpawnedEnemy->Destroy();
	}

	SpawnedEnemy = nullptr;

	Super::EndPlay(EndPlayReason);
}

void ACXFieldEnemySpawner::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (IsValid(PatrolRangePreview))
	{
		PatrolRangePreview->SetSphereRadius(
			FMath::Max(0.0f, PatrolRadius),
			false
		);
	}
}

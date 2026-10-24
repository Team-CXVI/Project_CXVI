// Fill out your copyright notice in the Description page of Project Settings.


#include "CXFieldEnemy.h"
#include "Interactions/CXPreemptiveAttackTriggerBox.h"
#include "CXVI/Foundation/FieldEnemySpawning/CXFieldEnemySubsystem.h"

#include "Engine/World.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldEnemy)

ACXFieldEnemy::ACXFieldEnemy( )
{
	PreemptiveAttackTrigger = CreateDefaultSubobject<UChildActorComponent>(TEXT("PreemptiveAttackTrigger"));
	
	PreemptiveAttackTrigger->SetupAttachment(GetRootComponent());

	PreemptiveAttackTrigger->SetChildActorClass(ACXPreemptiveAttackTriggerBox::StaticClass());

	PreemptiveAttackTrigger->SetRelativeLocation(FVector::ZeroVector);
	
	// 공격 반경 기본값 설정.
	AttackRadius = 50.0f;

	AIControllerClass = EnemyAIController;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

}

void ACXFieldEnemy::InitializePatrolRadius(float InPatrolRadius)
{
	PatrolRadius = FMath::Max(0.0f, InPatrolRadius);
}

void ACXFieldEnemy::InitializeSpawnId(FName InSpawnId)
{
	SpawnId = InSpawnId;
}

bool ACXFieldEnemy::Defeat()
{
	// 이미 처리했다면 다시 실행하지 않음.
	if (bDefeatHandled)
	{
		return true;
	}

	if (SpawnId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: SpawnId가 없어 처치를 기록할 수 없습니다."), *GetName());
		return false;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	UCXFieldEnemySubsystem* EnemySubsystem = World->GetSubsystem<UCXFieldEnemySubsystem>();
	if (!IsValid(EnemySubsystem))
	{
		return false;
	}

	// 기록이 실패했다면 액터를 먼저 제거하지 않음.
	if (!EnemySubsystem->MarkEnemyDefeated(SpawnId))
	{
		return false;
	}

	bDefeatHandled = true;

	Destroy();

	return true;
}

void ACXFieldEnemy::BeginPlay()
{
	Super::BeginPlay();

}

void ACXFieldEnemy::AttackEnded(UAnimMontage* Montage, bool bInterrupted)
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);

	// 공격이 끝나는 시점을 알 수 있도록 가상함수 호출.
	NotifyActionEnd();
}

void ACXFieldEnemy::NotifyActionEnd()
{
	// 공격 종료를 알리는 델리게이트 호출.
	OnAttackFinished.ExecuteIfBound();
}

float ACXFieldEnemy::GetAIPatrolRadius()
{
	return PatrolRadius;
}

float ACXFieldEnemy::GetAIDetectRange()
{
	return 400.0f;
}

float ACXFieldEnemy::GetAIAttackRange()
{
	return AttackRadius;
}

float ACXFieldEnemy::GetAITurnSpeed()
{
	return 0.0f;
}

void ACXFieldEnemy::AttackByAI()
{
	// 공격 실행.
	const float AttackSpeedRate = 1.0f;

	// 몽타주 재생.
	UAnimInstance* AnimInstane = GetMesh()->GetAnimInstance();
	AnimInstane->Montage_Play(AttackMontage, AttackSpeedRate);

	// 몽타주 종료 이벤트에 등록.
	FOnMontageEnded OnMontageEnded;
	OnMontageEnded.BindUObject(this, &ThisClass::AttackEnded);

	AnimInstane->Montage_SetEndDelegate(OnMontageEnded, AttackMontage);

	// 공격 모션 중에는 이동하지 않도록 설정.
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);



	// 공격 끝나는 시점을 알 필요가 있음.
	
	
}

void ACXFieldEnemy::SetAIAttackDelegate(const FAICharacterAttackFinished& InOnAttackFinished)
{
	// 델리게이트에 전달된 로직 저장.
	OnAttackFinished = InOnAttackFinished;
}



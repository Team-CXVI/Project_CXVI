// Fill out your copyright notice in the Description page of Project Settings.


#include "CXFieldEnemy.h"
#include "Interactions/CXPreemptiveAttackTriggerBox.h"
#include "CXVI/Foundation/FieldEnemySpawning/CXFieldEnemySubsystem.h"
#include "CXVI/AI/Field/CXFieldAI.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "Engine/World.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldEnemy)

ACXFieldEnemy::ACXFieldEnemy( )
{
	PrimaryActorTick.bCanEverTick = true;


	PreemptiveAttackTrigger = CreateDefaultSubobject<UChildActorComponent>(TEXT("PreemptiveAttackTrigger"));
	
	PreemptiveAttackTrigger->SetupAttachment(GetRootComponent());

	PreemptiveAttackTrigger->SetChildActorClass(ACXPreemptiveAttackTriggerBox::StaticClass());

	PreemptiveAttackTrigger->SetRelativeLocation(FVector::ZeroVector);
	
	// 공격 반경 기본값 설정.
	AttackRadius = 50.0f;

	AIControllerClass = EnemyAIController;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

}

void ACXFieldEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bTurningAfterAttack)
	{
		return;
	}

	AAIController* EnemyController = Cast<AAIController>(GetController());

	UBlackboardComponent* Blackboard = IsValid(EnemyController) ? EnemyController->GetBlackboardComponent() : nullptr;

	// 복귀 요청이 들어오면 방향 정렬을 끝내고 BT에 완료 알림.
	if (!Blackboard || Blackboard->GetValueAsBool(BBKEY_ISRETURNING))
	{
		FinishAttackAction();
		return;
	}

	const float NewYaw = FMath::FixedTurn(GetActorRotation().Yaw, AttackTargetYaw, FMath::Max(1.0f, AttackTurnSpeed) * DeltaSeconds);

	const FRotator NewRotation(0.0f, NewYaw, 0.0f);

	EnemyController->SetControlRotation(NewRotation);
	SetActorRotation(NewRotation);

	const float RemainingAngle = FMath::Abs(FMath::FindDeltaAngleDegrees(NewYaw, AttackTargetYaw));

	if (RemainingAngle <= 1.0f)
	{
		FinishAttackAction();
	}
}

void ACXFieldEnemy::InitializePatrolArea(const FVector& InPatrolCenter, float InPatrolRadius)
{
	PatrolCenter = InPatrolCenter;
	PatrolRadius = FMath::Max(0.0f, InPatrolRadius);
	bPatrolAreaInitialize = true;
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

void ACXFieldEnemy::InitializeEnemyGroupId(FName InEnemyGroupId)
{
	EnemyGroupId = InEnemyGroupId;
}

void ACXFieldEnemy::BeginPlay()
{
	Super::BeginPlay();

}

void ACXFieldEnemy::AttackEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bAIAttacking|| bTurningAfterAttack || Montage != AttackMontage.Get())
	{
		return;
	}

	// 정상 종료 후 회전이 필요하면, 완료 알림을 잠시 보류.
	if (!bInterrupted && FaceAttackTarget())
	{
		return;
	}


	FinishAttackAction();
}

void ACXFieldEnemy::NotifyActionEnd()
{
	// 호출 전에 저장된 콜백을 비워 중복 실행을 방지.
	FAICharacterAttackFinished FinishedCallback = OnAttackFinished;
	OnAttackFinished.Unbind();

	// 공격 종료를 알리는 델리게이트 호출.
	FinishedCallback.ExecuteIfBound();
}

bool ACXFieldEnemy::FaceAttackTarget()
{
	AAIController* EnemyController = Cast<AAIController>(GetController());

	UBlackboardComponent* Blackboard = IsValid(EnemyController) ? EnemyController->GetBlackboardComponent() : nullptr;
	if (!Blackboard)
	{
		return false;
	}

	// 복귀 중이거나 타깃이 보이지 않으면 방향을 갱신하지 않음.
	if (Blackboard->GetValueAsBool(BBKEY_ISRETURNING) || !Blackboard->GetValueAsBool(BBEKY_HASTARGETSIGHT))
	{
		return false;
	}


	AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(BBKEY_TARGET));
	if (!IsValid(Target) || GetDistanceTo(Target) > GetAIAttackRange())
	{
		return false;
	}

	FVector Direction = Target->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.0f;

	if (Direction.IsNearlyZero())
	{
		return false;
	}

	AttackTargetYaw = Direction.Rotation().Yaw;

	const float AngleDifference = FMath::Abs(FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, AttackTargetYaw));

	if (AngleDifference <= 1.0f)
	{
		return false;
	}
	
	// 여기서 바로 회전 하지 않고 Tick에서 회전.
	bTurningAfterAttack = true;
	return true;
}

void ACXFieldEnemy::FinishAttackAction()
{
	if (!bAIAttacking)
	{
		return;
	}

	bTurningAfterAttack = false;
	bAIAttacking = false;

	UCharacterMovementComponent* EnemyMovement = GetCharacterMovement();
	if (IsValid(EnemyMovement) && EnemyMovement->MovementMode == EMovementMode::MOVE_None)
	{
		EnemyMovement->SetMovementMode(EMovementMode::MOVE_Walking);
	}
	
	NotifyActionEnd();
}

float ACXFieldEnemy::GetAIPatrolRadius()
{
	return PatrolRadius;
}

FVector ACXFieldEnemy::GetAIPatrolCenter()
{
	return PatrolCenter;
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

float ACXFieldEnemy::GetAIChaseRadius()
{
	return PatrolRadius + FMath::Max(0.0f, ExtraChaseDistance);
}

bool ACXFieldEnemy::AttackByAI()
{
	// 이미 진행 중인 AI 공격은 중복 시작하지 않음.
	if (bAIAttacking)
	{
		return false;
	}

	USkeletalMeshComponent* EnemyMesh = GetMesh();
	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if (!IsValid(EnemyMesh) || !IsValid(Movement) || !IsValid(AttackMontage))
	{
		return false;
	}
	
	// 몽타주 재생.
	UAnimInstance* AnimInstane = EnemyMesh->GetAnimInstance();
	if (!IsValid(AnimInstane))
	{
		return false;
	}

	// 공격 실행.
	const float AttackSpeedRate = 1.0f;

	const float PlayResult = AnimInstane->Montage_Play(AttackMontage, AttackSpeedRate);
	if (PlayResult <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Attack] %s: 공격 몽타주 재생 실패"), *GetName());

		return false;
	}

	// 몽타주 종료 이벤트에 등록.
	FOnMontageEnded OnMontageEnded;
	OnMontageEnded.BindUObject(this, &ThisClass::AttackEnded);

	AnimInstane->Montage_SetEndDelegate(OnMontageEnded, AttackMontage);

	// 공격 모션 중에는 이동하지 않도록 설정.
	bAIAttacking = true;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);

	return true;
}

void ACXFieldEnemy::CancelAttackByAI()
{
	// 취소된 Task에 완료 알림을 보내지 않음.
	OnAttackFinished.Unbind();

	// 강제 취소 시 프레임별 회전도 중단.
	bTurningAfterAttack = false;

	if (!bAIAttacking)
	{
		return;
	}

	bAIAttacking = false;

	USkeletalMeshComponent* EnemyMesh = GetMesh();
	UAnimInstance* AnimInstance = IsValid(EnemyMesh) ? EnemyMesh->GetAnimInstance() : nullptr;

	if (IsValid(AnimInstance) && IsValid(AttackMontage) && AnimInstance->Montage_IsActive(AttackMontage))
	{
		// 종료 콜백을 제거하고 공격 몽타주만 즉시 중단.
		FOnMontageEnded EmptyDelegate;

		AnimInstance->Montage_SetEndDelegate(EmptyDelegate, AttackMontage);

		AnimInstance->Montage_Stop(0.0f, AttackMontage);
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if (IsValid(Movement) && Movement->MovementMode == EMovementMode::MOVE_None)
	{
		Movement->SetMovementMode(EMovementMode::MOVE_Walking);
	}
}

bool ACXFieldEnemy::IsAIAttacking() const
{
	return bAIAttacking;
}

void ACXFieldEnemy::SetAIAttackDelegate(const FAICharacterAttackFinished& InOnAttackFinished)
{
	// 델리게이트에 전달된 로직 저장.
	OnAttackFinished = InOnAttackFinished;
}

float ACXFieldEnemy::GetAISightHalfAngle()
{
	return FMath::Clamp(SightHalfAngle,0.0f,180.0f);
}

float ACXFieldEnemy::GetAISightLossGraceTime()
{
	return FMath::Max(0.0f,SightLossGraceTime);
}



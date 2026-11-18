// Fill out your copyright notice in the Description page of Project Settings.


#include "CXFieldEnemy.h"
#include "CXVI/Foundation/Field/FieldEnemySpawning/CXFieldEnemySubsystem.h"
#include "CXVI/AI/Field/CXFieldAI.h"
#include "Character/Field/CXFIeldPlayerCharacter.h"
#include "CXVI/Foundation/Scene/CXSceneSubsystem.h"
#include "CXVI/Foundation/Scene/Scenes/CXFieldScene.h"
#include "CXVI/AI/Field/CXFieldAIController.h"
#include "TimerManager.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#include "Foundation/Combat/CXCombatManagerSubsystem.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldEnemy)

ACXFieldEnemy::ACXFieldEnemy( )
{
	PrimaryActorTick.bCanEverTick = true;


	// 공격 반경 기본값 설정.
	AttackRadius = 50.0f;

	AIControllerClass = EnemyAIController;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 라이팅 채널 : 0(기본 라이트) + 1(필드 캐릭터 전용 라이트, 필드 레벨의 DL_Field_CharacterKey). 플레이어와 같은 채널
	GetMesh()->SetLightingChannels(true, true, false);
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

void ACXFieldEnemy::PauseAIForSeconds(float Duration)
{
	ACXFieldAIController* AIController = Cast<ACXFieldAIController>(GetController());
	if (!IsValid(AIController))
	{
		return;
	}

	AIController->StopAI();
	AIController->StopMovement();
	bPlayerCollisionIgnored = true;

	GetWorldTimerManager().SetTimer(AIPauseTimerHandle, this, &ThisClass::ResumeAI, Duration, false);
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

void ACXFieldEnemy::InitializeEncounterId(FName InEnemyGroupId)
{
	EncounterId = InEnemyGroupId;
}

void ACXFieldEnemy::NotifyAttackHit()
{
	// 취소됐거나 이미 처치된 적의 타격은 무시.
	if (!bAIAttacking || bDefeatHandled || bTurningAfterAttack)
	{
		return;
	}

	UWorld* World = GetWorld();
	AAIController* EnemyController = Cast<AAIController>(GetController());
	UBlackboardComponent* Blackboard = IsValid(EnemyController) ? EnemyController->GetBlackboardComponent() : nullptr;
	if (!World || !IsValid(Blackboard))
	{
		return;
	}


	// 복귀 중인 적은 타격을 인정하지 않음.
	if (Blackboard->GetValueAsBool(BBKEY_ISRETURNING))
	{
		return;
	}

	// 적이 탐지해서 유지하고 있는 플레이어 타깃.
	ACXFieldPlayerCharacter* TargetPlayer = Cast<ACXFieldPlayerCharacter>(Blackboard->GetValueAsObject(BBKEY_TARGET));
	if (!IsValid(TargetPlayer))
	{
		return;
	}

	const float Range = FMath::Max(2.0f, AttackHitRange);
	const float Radius = FMath::Clamp(AttackHitRadius, 1.0f, Range * 0.5f);

	const FVector Forward = GetActorForwardVector();
	const FVector Origin = GetActorLocation();

	const FVector Start = Origin + Forward * Radius;
	const FVector End = Origin + Forward * (Range - Radius);

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(FieldEnemyAttack),
		false,
		this
	);

	FHitResult Hit;

	const bool bHit = World->SweepSingleByChannel(
		Hit,
		Start,
		End,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(Radius),
		QueryParams
	);
	
	// 실제로 맞은 액터가 추적하던 플레이어인지 확인.
	const bool bHitTarget = bHit && Hit.GetActor() == TargetPlayer;

	if (bDrawAttackHitDebug)
	{
		DrawDebugCapsule(
			World,
			(Start + End) * 0.5f,
			FVector::Distance(Start, End) * 0.5f + Radius,
			Radius,
			FRotationMatrix::MakeFromZ(Forward).ToQuat(),
			bHitTarget ? FColor::Green : FColor::Red,
			false,
			1.0f);
	}

	if (!bHitTarget)
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyAttack] 명중 실패 Target=%s Blocker=%s"), *GetNameSafe(TargetPlayer), *GetNameSafe(Hit.GetActor()));
		return;
	}

	UE_LOG(LogTemp, Log,
		TEXT("[EnemyAttack] 플레이어 명중 Enemy=%s Player=%s Encounter=%s Spawn=%s"),
		*GetName(),
		*TargetPlayer->GetName(),
		*GetEncounterId().ToString(),
		*GetSpawnId().ToString());

	SuccessedAttackHit(TargetPlayer);
}

void ACXFieldEnemy::SuccessedAttackHit(ACXFieldPlayerCharacter* Player)
{
	UWorld* World = GetWorld();
	if (!IsValid(Player) || !World)
	{
		return;
	}

	/*=================================에너미 공격======================================*/
	// 전투에 들어갈 수 있는 상태에서만 연출.
	UCXSceneSubsystem* SceneSubsystem = World->GetSubsystem<UCXSceneSubsystem>();
	if (!IsValid(SceneSubsystem) || SceneSubsystem->IsTransitioning())
	{
		return;
	}

	UCXFieldScene* FieldScene = Cast<UCXFieldScene>(SceneSubsystem->GetCurrentScene());
	if (!IsValid(FieldScene))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Encounter] 현재 씬이 필드 씬이 아닙니다."));
		return;
	}

	const float Dilation = 0.1f;
	const float RealDuration = 1.0f;
	const float PullBack = 250.0f;
	const float OrbitYaw = 45.0f;

	// 연출과 씬 전환(에셋 로드)이 끝날 때까지 몬스터가 움직이거나 다시 전투를 걸지 않도록 전체 정지.
	SetFieldEnemiesSuspended(World, true);

	UGameplayStatics::SetGlobalTimeDilation(World, Dilation);

	if (USpringArmComponent* Arm = Player->FindComponentByClass<USpringArmComponent>())
	{
		Arm->bEnableCameraLag = true;
		Arm->CameraLagSpeed = 3.0f / Dilation;  // 슬로우 중에도 체감 속도 유지.
		Arm->TargetArmLength += PullBack;

		// 회전도 부드럽게 따라가도록 회전 랙 사용 (슬로우 보정).
		Arm->bEnableCameraRotationLag = true;
		Arm->CameraRotationLagSpeed = 3.0f / Dilation;
	}

	// 위에서 볼 때 Yaw 감소 = 반시계 방향.
	if (AController* PlayerController = Player->GetController())
	{
		FRotator Rot = PlayerController->GetControlRotation();
		Rot.Yaw -= OrbitYaw;
		PlayerController->SetControlRotation(Rot);
	}

	// 타이머도 슬로우 영향을 받으므로 실제 시간 × 배율로 건다.
	TWeakObjectPtr<ACXFieldPlayerCharacter> WeakPlayer = Player;
	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(
		this, [this, World, WeakPlayer]()
		{
			UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);

			if (!RequestEnemyAttackBattle(World))
			{
				// 전투 진입이 거절되면 원래 상태로 되돌림.
				SetFieldEnemiesSuspended(World, false);
				if (WeakPlayer.IsValid())
				{
					WeakPlayer->ResetCameraArm();
				}
			}
			// 성공하면 카메라는 화면이 어두워진 뒤(플레이어 OnSceneSuspended)에 되돌린다.
		}
	), RealDuration * Dilation, false);
}

void ACXFieldEnemy::SetFieldEnemiesSuspended(UWorld* World, bool bSuspended) const
{
	for (TActorIterator<ACXFieldEnemy> It(World); It; ++It)
	{
		if (bSuspended)
		{
			ICXSceneSuspendable::Execute_OnSceneSuspended(*It);
		}
		else
		{
			ICXSceneSuspendable::Execute_OnSceneResumed(*It);
		}
	}
}

bool ACXFieldEnemy::RequestEnemyAttackBattle(UWorld* World)
{
	// TODO : 전투에 필요한 캐릭터 정리 (파티 정보 출처가 정해지기 전에는 레벨 정의의 테스트 파티로 진행)
	UCXCombatManagerSubsystem* Subsystem = UCXCombatManagerSubsystem::TryGet(World);
	if (!Subsystem)
	{
		return false;
	}

	FCXCombatEnterParams Params;
	Params.EncounterId = GetEncounterId();
	Params.bIsPlayerAttack = false;
	UE_LOG(LogTemp, Log, TEXT("플레이어 선공: %s"), Params.bIsPlayerAttack ? TEXT("true") : TEXT("false"));

	const bool bRequested = Subsystem->RequestStartBattle(Params);

	if (UCXFieldEnemySubsystem* EnemySubsystem = World->GetSubsystem<UCXFieldEnemySubsystem>())
	{
		EnemySubsystem->SetSpawnedId(GetSpawnId());
	}

	return bRequested;
}

void ACXFieldEnemy::OnSceneSuspended_Implementation()
{
	// 씬이 멈춰도 월드 타이머는 돌아가므로, 예약된 ResumeAI가 전투 중에 AI를 깨우지 않게 취소.
	GetWorldTimerManager().ClearTimer(AIPauseTimerHandle);

	ACXFieldAIController* AIController = Cast<ACXFieldAIController>(GetController());
	if (!IsValid(AIController))
	{
		return;
	}

	AIController->StopAI();
	AIController->StopMovement();
}

void ACXFieldEnemy::OnSceneResumed_Implementation()
{
	// 전투에서 돌아오면(도망 등) 바로 감지하지 않고 3초 뒤 ResumeAI에서 다시 시작.
	PauseAIForSeconds(3.0f);
}

void ACXFieldEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	PauseAIForSeconds(3.0f);

	// 첫 Idle 사운드 예약. 시점이 랜덤이라 여러 마리가 동시에 울지 않음.
	if (IsValid(IdleSound))
	{
		GetWorldTimerManager().SetTimer(IdleSoundTimerHandle, this, &ThisClass::PlayIdleSound,
			FMath::FRandRange(IdleSoundMinInterval, IdleSoundMaxInterval), false);
	}
}

void ACXFieldEnemy::PlayIdleSound()
{
	// 플레이어를 쫓는 중이 아닐 때만 재생.
	const AAIController* AIController = Cast<AAIController>(GetController());
	const UBlackboardComponent* Blackboard = IsValid(AIController) ? AIController->GetBlackboardComponent() : nullptr;

	if (Blackboard && Blackboard->GetValueAsObject(BBKEY_TARGET) == nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, IdleSound, GetActorLocation());
	}

	// 다음 재생 예약.
	GetWorldTimerManager().SetTimer(IdleSoundTimerHandle, this, &ThisClass::PlayIdleSound,
		FMath::FRandRange(IdleSoundMinInterval, IdleSoundMaxInterval), false);
}

void ACXFieldEnemy::OnAITargetDetected()
{
	if (IsValid(DetectSound))
	{
		UGameplayStatics::PlaySoundAtLocation(this, DetectSound, GetActorLocation());
	}
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

void ACXFieldEnemy::ResumeAI()
{
	if (ACXFieldAIController* AIController = Cast<ACXFieldAIController>(GetController()))
	{
		AIController->RunAI();
		bPlayerCollisionIgnored = false;
	}
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
	return DetectRange;
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
	const float AttackSpeedRate = 0.5f;

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



// Fill out your copyright notice in the Description page of Project Settings.


#include "CXGA_FieldPlayerAttack.h"
#include "Tags/CXFieldTags.h"
#include "Character/Field/CXFieldPlayerCharacter.h"
#include "Character/Field/CXFieldEnemy.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "CXVI/Foundation/Scene/CXSceneSubsystem.h"
#include "CXVI/Foundation/Scene/Scenes/CXFieldScene.h"
#include "CXVI/Foundation/Scene/CXSceneSubsystem.h"
#include "CXVI/Foundation/Field/FieldEnemySpawning/CXFieldEnemySubsystem.h"
#include "CXVI/Foundation/Scene/CXSceneSuspendable.h"

#include "Foundation/Combat/CXCombatManagerSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGA_FieldPlayerAttack)

UCXGA_FieldPlayerAttack::UCXGA_FieldPlayerAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	ActivationOwnedTags.AddTag(CXVIFieldTag::Input_Field_Attack);
	ActivationOwnedTags.AddTag(CXVIFieldTag::State_MovingBlock);

	ActivationBlockedTags.AddTag(CXVIFieldTag::State_MovingBlock);
	ActivationBlockedTags.AddTag(CXVIFieldTag::State_Airborne);
}

bool UCXGA_FieldPlayerAttack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(ActorInfo->AvatarActor.Get());
	UCharacterMovementComponent* PlayerMovement = Player->GetCharacterMovement();

	return IsValid(ASC) && IsValid(Player) && IsValid(PlayerMovement) && !PlayerMovement->IsCrouching();
}

void UCXGA_FieldPlayerAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(ActorInfo->AvatarActor.Get());

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	UAbilityTask_WaitGameplayEvent* AttackHitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		CXVIFieldTag::Event_Field_AttackHit, 
		nullptr, // 이 GA의 ASC에서 수신.
		true,	 // 이번 활성화에서는 한 번만 수신
		true);	 // 정확히 같은 태그만 수신.

	AttackHitTask->EventReceived.AddDynamic(this, &ThisClass::OnAttackHitEvent);
	AttackHitTask->ReadyForActivation();

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AttackMontage, 1.0f, NAME_None, true);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnAttackInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnAttackInterrupted);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnAttackCompleted);
	MontageTask->ReadyForActivation();

	if (!IsActive())
	{
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("어택어빌리티 성공"));
}

void UCXGA_FieldPlayerAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo))
	{
		return;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UCXGA_FieldPlayerAttack::OnAttackHitEvent(FGameplayEventData Payload)
{
	if (!IsActive())
	{
		return;
	}
	
	AActor* Player = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();

	if (!IsValid(Player) || !World)
	{
		return;
	}

	const float Range = FMath::Max(2.0f, AttackRange);
	const float Radius = FMath::Clamp(AttackRadius, 1.0f, Range * 0.5f);

	const FVector Forward = Player->GetActorForwardVector();
	const FVector Origin = Player->GetActorLocation();

	// 구체 중심이 아닌 판정 전체가 전방 0~Range에 들어가도록 배치.
	const FVector Start = Origin + Forward * Radius;
	const FVector End = Origin + Forward * (Range - Radius);

	// 자기 자신은 검사에서 제외.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FieldAttack), false, Player);

	FHitResult Hit;

	const bool bHit = World->SweepSingleByChannel(
		Hit,
		Start,
		End, 
		FQuat::Identity, 
		ECC_Visibility,
		FCollisionShape::MakeSphere(Radius),
		QueryParams);

	ACXFieldEnemy* Enemy = bHit ? Cast<ACXFieldEnemy>(Hit.GetActor()) : nullptr;
	
	// AI 정지 중에는 에너미 타격이 불가능.
	if (IsValid(Enemy) && !Enemy->CanBeAttacked())
	{
		Enemy = nullptr;
	}
	
	// 드로우 디버그 모드이면.
	if (bDrawAttackDebug)
	{
		const FVector Center = (Start + End) * 0.5f;
		const float HalfHeight =
			FVector::Distance(Start, End) * 0.5f + Radius;

		DrawDebugCapsule(
			World,
			Center,
			HalfHeight,
			Radius,
			FRotationMatrix::MakeFromZ(Forward).ToQuat(),
			IsValid(Enemy) ? FColor::Green : FColor::Red,
			false,
			1.0f);
	}

	if (!IsValid(Enemy))
	{
		UE_LOG(LogTemp, Log, TEXT("[FieldAttack] 적 명중 없음. Blocker:%s"), *GetNameSafe(Hit.GetActor()));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[FieldAttack] 명중 Enemy=%s Group=%s Spawn=%s"),*Enemy->GetName(),*Enemy->GetEncounterId().ToString(), *Enemy->GetSpawnId().ToString());

	ACXFieldPlayerCharacter* PlayerCharacter = Cast<ACXFieldPlayerCharacter>(Player);
	SuccessedAttackHit(PlayerCharacter,Enemy);

}

void UCXGA_FieldPlayerAttack::SetFieldEnemiesSuspended(UWorld* World, bool bSuspended) const
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

bool UCXGA_FieldPlayerAttack::RequestPlayerAttackBattle(UWorld* World, ACXFieldEnemy* Enemy) const
{
	if (!IsValid(Enemy))
	{
		return false;
	}

	UCXCombatManagerSubsystem* Subsystem = UCXCombatManagerSubsystem::TryGet(World);
	if (!Subsystem)
	{
		return false;
	}

	FCXCombatEnterParams Params;
	Params.EncounterId = Enemy->GetEncounterId();
	Params.bIsPlayerAttack = true;

	UE_LOG(LogTemp, Log, TEXT("플레이어 선공: %s"), Params.bIsPlayerAttack ? TEXT("true") : TEXT("false"));

	const bool bRequested = Subsystem->RequestStartBattle(Params);

	if (UCXFieldEnemySubsystem* EnemySubsystem = World->GetSubsystem<UCXFieldEnemySubsystem>())
	{
		EnemySubsystem->SetSpawnedId(Enemy->GetSpawnId());
	}

	return bRequested;
}

void UCXGA_FieldPlayerAttack::SuccessedAttackHit(ACXFieldPlayerCharacter* Player,ACXFieldEnemy* Enemy)
{
	UWorld* World = GetWorld();
	if (!IsValid(Player) || !IsValid(Enemy) || !World)
	{
		return;
	}

	/*=================================플레이어 공격======================================*/
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

	UGameplayStatics::SetGlobalTimeDilation(World,Dilation);

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
	if (AController* Controller = Player->GetController())
	{
		FRotator Rot = Controller->GetControlRotation();
		Rot.Yaw -= OrbitYaw;
		Controller->SetControlRotation(Rot);
	}

	// 타이머도 슬로우 영향을 받으므로 실제 시간 × 배율로 건다.
	// 어빌리티는 InstancedPerActor라 몽타주가 끝나 종료돼도 객체는 남아 있음.
	TWeakObjectPtr<ACXFieldPlayerCharacter> WeakPlayer = Player;
	TWeakObjectPtr<ACXFieldEnemy> WeakEnemy = Enemy;
	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(
		this, [this, World, WeakPlayer, WeakEnemy]()
		{
			UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);

			if (!RequestPlayerAttackBattle(World, WeakEnemy.Get()))
			{
				// 전투 진입이 거절되면 원래 상태로 되돌림.
				SetFieldEnemiesSuspended(World, false);
				if (WeakPlayer.IsValid())
				{
					WeakPlayer->ResetCameraArm();
				}
			}
			// 성공하면 카메라는 화면이 어두워진 뒤(OnSceneSuspended)에 되돌린다.
		}
	), RealDuration * Dilation, false);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "CXBTService_FieldDetect.h"
#include "Interface/CXFieldEnemyAIInterface.h"
#include "CXFieldAI.h"

#include "AIController.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "BehaviorTree/BlackboardComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXBTService_FieldDetect)

UCXBTService_FieldDetect::UCXBTService_FieldDetect()
{
	// 에디터에 보이는 노드 이름 값.
	NodeName = TEXT("Detect");

	// 서비스가 실행될 간격(단위: 초)
	Interval = 1.0f;
}

void UCXBTService_FieldDetect::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	// 충돌 기능을 활용하기 위해 월드 객체 얻어오기.
	APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (!IsValid(ControllingPawn))
	{
		return;
	}
	
	// 충돌 기능을 활용해 캐릭터 감지.
	UWorld* World = ControllingPawn->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	//충돌 범위 가져올 인터페이스.
	ICXFieldEnemyAIInterface* AIPawn = Cast<ICXFieldEnemyAIInterface>(ControllingPawn);
	if (nullptr == AIPawn)
	{
		return;
	}

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return;
	}

	// 복귀 중에는 플레이어를 다시 감지하지 않음.
	if (Blackboard->GetValueAsBool(BBKEY_ISRETURNING))
	{
		Blackboard->SetValueAsBool(BBEKY_HASTARGETSIGHT, false);

		Blackboard->ClearValue(BBKEY_LASTSEENTARGETPOS);

		Blackboard->SetValueAsFloat(BBKEY_SIGHTLOSTSTARTTIME, -1.0f);

		Blackboard->ClearValue(BBKEY_TARGET);
		return;
	}

	const FVector PatrolCenter = AIPawn->GetAIPatrolCenter();
	const float ChaseRadius = AIPawn->GetAIChaseRadius();
	const float ChaseRadiusSquared = FMath::Square(ChaseRadius);

	AActor* CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(BBKEY_TARGET));

	const bool bCanSeeCurrentTarget = IsValid(CurrentTarget) && CanSeeTarget(ControllingPawn, CurrentTarget, AIPawn->GetAISightHalfAngle());

	// Enemy와 타깃 각각 담당 구역 중심으로부터의 거리검사.
	const bool bEnemyOutside = FVector::DistSquared(ControllingPawn->GetActorLocation(), PatrolCenter) > ChaseRadiusSquared;

	const bool bTargetOutside = bCanSeeCurrentTarget && FVector::DistSquared(CurrentTarget->GetActorLocation(), PatrolCenter) > ChaseRadiusSquared;


	if (bEnemyOutside || bTargetOutside)
	{
		Blackboard->SetValueAsBool(BBEKY_HASTARGETSIGHT, false);

		Blackboard->ClearValue(BBKEY_LASTSEENTARGETPOS);

		Blackboard->SetValueAsFloat(BBKEY_SIGHTLOSTSTARTTIME, -1.0f);

		// 복귀 상태를 먼저 설정한 후 타깃 해제.
		Blackboard->SetValueAsBool(BBKEY_ISRETURNING, true);
		Blackboard->ClearValue(BBKEY_TARGET);

		UE_LOG(
			LogTemp,
			Display,
			TEXT("[Chase] %s: 추적 한계 초과, 복귀 시작. EnemyOutside=%d TargetOutside=%d"),
			*ControllingPawn->GetName(), bEnemyOutside, bTargetOutside
		);

		return;
	}

	// 이미 잡은 타깃은 추적 한계 안에 있으면 유지.
	// 최초 감지 반경을 벗어났다고 즉시 지우지 않음.

	if (IsValid(CurrentTarget))
	{
		const float LostStartTime = Blackboard->GetValueAsFloat(BBKEY_SIGHTLOSTSTARTTIME);

		// 다시 보이면 시야 상실 기록 초기화.
		if (bCanSeeCurrentTarget)
		{
			Blackboard->SetValueAsVector(BBKEY_LASTSEENTARGETPOS, CurrentTarget->GetActorLocation());

			Blackboard->SetValueAsBool(BBEKY_HASTARGETSIGHT, true);

			Blackboard->SetValueAsFloat(BBKEY_SIGHTLOSTSTARTTIME, -1.0f);
			return;
		}

		// 보이지 않으면 위치를 갱신하지 않고 마지막 기록 유지.
		Blackboard->SetValueAsBool(BBEKY_HASTARGETSIGHT, false);

		const float Now = World->GetTimeSeconds();
		float StartTime = LostStartTime;

		// 안 보이는 것을 처음 확인한 시점.
		if (StartTime < 0.0f)
		{
			StartTime = Now;

			Blackboard->SetValueAsFloat(BBKEY_SIGHTLOSTSTARTTIME, StartTime);

			UE_LOG(LogTemp, Display, TEXT("[Sight] %s: 타깃 시야 상실. 유예 시작"), *ControllingPawn->GetName());
		}

		// 유예 시간이 지나면 추적을 포기하고 복귀.
		if (Now - StartTime >= AIPawn->GetAISightLossGraceTime())
		{
			Blackboard->SetValueAsBool(BBEKY_HASTARGETSIGHT, false);

			Blackboard->ClearValue(BBKEY_LASTSEENTARGETPOS);

			Blackboard->SetValueAsFloat(BBKEY_SIGHTLOSTSTARTTIME, -1.0f);

			Blackboard->SetValueAsBool(BBKEY_ISRETURNING, true);
			Blackboard->ClearValue(BBKEY_TARGET);

			UE_LOG(LogTemp, Display, TEXT("[Sight] %s: 시야 상실 유예 초과. 복귀 시작"), *ControllingPawn->GetName());
		}

		// 유예 중에는 기존 Target유지.
		return;
	}

	// 타깃이 없으면 이전 시야 상실 기록 제거.
	Blackboard->SetValueAsFloat(BBKEY_SIGHTLOSTSTARTTIME, -1.0f);

	Blackboard->SetValueAsBool(BBEKY_HASTARGETSIGHT, false);

	Blackboard->ClearValue(BBKEY_LASTSEENTARGETPOS);


	// 충돌 결과로 정보를 반환받을 변수.
	TArray<FOverlapResult> OverlapResults;

	// 충돌 감지를 시작할 위치 = 제어하는 폰의 위치.
	FVector Center = ControllingPawn->GetActorLocation();

	// 충돌 감지할 반경.
	float DetectRadius = AIPawn->GetAIDetectRange();

	// 콜리전 파라미터.
	FCollisionQueryParams Params(
		SCENE_QUERY_STAT(Detect),
		false,
		ControllingPawn
	);

	//ECC_GameTraceChannel1
	// 충돌 기능 활용.
	bool bResult = World->OverlapMultiByChannel(
		OverlapResults,
		Center,
		FQuat::Identity,
		ECC_GameTraceChannel1,
		FCollisionShape::MakeSphere(DetectRadius),
		Params
		);

	// 충돌 감지 됐을 때 처리.
	if (bResult)
	{
		// 감지된 플레이어에 대한 처리(루프로 진행).
		for (const auto& OverlapResult : OverlapResults)
		{
			// 충돌에서 감지한 액터를 폰으로 형변환.
			APawn* Pawn = Cast<APawn>(OverlapResult.GetActor());

			// 플레이어인지 확인 -> 폰을 제어하는 컨트롤러가 플레이어 컨트롤러인지 확인.
			if (IsValid(Pawn) && IsValid(Pawn->GetController()) && Pawn->GetController()->IsPlayerController())
			{
				if (FVector::DistSquared(Pawn->GetActorLocation(), PatrolCenter) > ChaseRadiusSquared)
				{
					continue;
				}

				if (!CanSeeTarget(ControllingPawn, Pawn, AIPawn->GetAISightHalfAngle()))
				{
					continue;
				}

				// Target 변경으로 BT가 반응하기 전에 관련 정보부터 준비.
				Blackboard->SetValueAsVector(BBKEY_LASTSEENTARGETPOS, Pawn->GetActorLocation());

				Blackboard->SetValueAsBool(BBEKY_HASTARGETSIGHT, true);

				Blackboard->SetValueAsFloat(BBKEY_SIGHTLOSTSTARTTIME, -1.0f);

				// 감지된 정보를 블랙보드에 저장.
				OwnerComp.GetBlackboardComponent()->SetValueAsObject(
					BBKEY_TARGET,
					Pawn
				);

				// 디버그 드로우를 활용해서 시가적으로 표시.
				DrawDebugSphere(World, Center, DetectRadius, 16, FColor::Green, false, 0.2f);

				// 감지된 지점 표시.
				DrawDebugPoint(World, Pawn->GetActorLocation(), 10.0f, FColor::Green, false, 0.2f);

				// 감지된 방향을 선으로 표시.
				DrawDebugLine(World, ControllingPawn->GetActorLocation(), Pawn->GetActorLocation(), FColor::Green, false, 0.3f);

				return;
			}
		}
	}

	// 충돌 감지를 못한 경우에는 타깃 액터를 null로 지정.
	OwnerComp.GetBlackboardComponent()->SetValueAsObject(
		BBKEY_TARGET,
		nullptr
	);

	// 디버그 드로우를 활용해서 시가적으로 표시.
	DrawDebugSphere(World, Center, DetectRadius, 16, FColor::Red, false, 0.2f);
}

bool UCXBTService_FieldDetect::CanSeeTarget(APawn* Observer, AActor* Target, float SightHalfAngle) const
{
	if (!IsValid(Observer) || !IsValid(Target))
	{
		return false;
	}

	UWorld* World = Observer->GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const FVector ToTarget = Target->GetActorLocation() - Observer->GetActorLocation();

	if (ToTarget.SizeSquared2D() > FMath::Square(KINDA_SMALL_NUMBER))
	{
		const FVector Forward = Observer->GetActorForwardVector().GetSafeNormal2D();

		const float FacingDot = FVector::DotProduct(Forward, ToTarget.GetSafeNormal2D());

		const float CosThreshold = FMath::Cos(FMath::DegreesToRadians(SightHalfAngle));

		if (FacingDot < CosThreshold)
		{
			return false;
		}
	}
	
	FVector SightStart;
	FRotator ObserverRotation;
	Observer->GetActorEyesViewPoint(SightStart, ObserverRotation);

	FVector SightEnd;
	FRotator TargetRotation;
	Target->GetActorEyesViewPoint(SightEnd, TargetRotation);

	FCollisionQueryParams SightParams(SCENE_QUERY_STAT(FieldEnemySight), false, Observer);

	FHitResult SightHit;

	const bool bHit = World->LineTraceSingleByChannel(SightHit, SightStart, SightEnd, ECC_Visibility, SightParams);

	return !bHit||SightHit.GetActor()==Target;
}

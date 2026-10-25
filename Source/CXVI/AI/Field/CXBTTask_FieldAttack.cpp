// Fill out your copyright notice in the Description page of Project Settings.


#include "CXBTTask_FieldAttack.h"
#include "Interface/CXFieldEnemyAIInterface.h"	
#include "CXFieldAI.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXBTTask_FieldAttack)

UCXBTTask_FieldAttack::UCXBTTask_FieldAttack()
{
	NodeName = TEXT("FieldAttack");

	bCreateNodeInstance = true;
}

EBTNodeResult::Type UCXBTTask_FieldAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 감지된 폰에게 공격을 받았다는 메시지 전달.
	APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (!IsValid(ControllingPawn))
	{
		return EBTNodeResult::Failed;
	}

	// 메시지 전달을 위해서 인터페이스로 형변환.
	ICXFieldEnemyAIInterface* AIPawn = Cast<ICXFieldEnemyAIInterface>(ControllingPawn);
	if (nullptr == AIPawn)
	{
		return EBTNodeResult::Failed;
	}


	// 공격이 종료될 때 실행될 로직을 델리게이트로 전달.
	FAICharacterAttackFinished OnAttackFinished;

	// 델리게이트에 로직 바인딩.
	OnAttackFinished.BindLambda(
		[this, &OwnerComp]()
		{
			const EBTTaskStatus::Type Status = OwnerComp.GetTaskStatus(this);
			if (Status == EBTTaskStatus::Aborting)
			{
				// 몽타주가 끝났으므로 중단 완료 -> 복귀 허용.
				FinishLatentAbort(OwnerComp);
			}
			else if (Status == EBTTaskStatus::Active)
			{
				// 일반적인 공격 완료.
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			}
			// 공격이 종료되면 최종 태스크 종료.

		}
	);

	// Enemy 캐릭터에 공격 종료 시 실행할 로직 전달.
	AIPawn->SetAIAttackDelegate(OnAttackFinished);

	// 공격 메시지 전달.
	if (!AIPawn->AttackByAI())
	{
		// 시작에 실패했으므로 완료 콜백을 기다리지 않음.
		AIPawn->SetAIAttackDelegate(FAICharacterAttackFinished());

		return EBTNodeResult::Failed;
	}

	// 공격 명령을 실행한 후에 곧바로 종료해도 되는지 알 수 없기 때문에
	// InProgress(진행중) 반환.
		return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UCXBTTask_FieldAttack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();

	if (IsValid(ControllingPawn))
	{
		ICXFieldEnemyAIInterface* AIPawn = Cast<ICXFieldEnemyAIInterface>(ControllingPawn);

		if (!AIPawn)
		{
			return EBTNodeResult::Aborted;
		}

		UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

		const bool bReturning = Blackboard && Blackboard->GetValueAsBool(BBKEY_ISRETURNING);
		if (bReturning && AIPawn->IsAIAttacking())
		{
			// 공격을 끊지 않고 종료 콜백을 기다림.
			return EBTNodeResult::InProgress;
		}

		AIPawn->CancelAttackByAI();

		return EBTNodeResult::Aborted;
	}

	return EBTNodeResult::Aborted;
}

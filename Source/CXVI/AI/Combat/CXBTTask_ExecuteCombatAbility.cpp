// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Combat/CXBTTask_ExecuteCombatAbility.h"

#include "Character/Combat/CXCombatCharacterBase.h"
#include "AIController.h"

#include "BehaviorTree/BehaviorTreeComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXBTTask_ExecuteCombatAbility)

UCXBTTask_ExecuteCombatAbility::UCXBTTask_ExecuteCombatAbility()
{
	NodeName = TEXT("Execute Combat Ability");

	bCreateNodeInstance = true;
	bNotifyTick = true;
	bNotifyTaskFinished = true;
}

EBTNodeResult::Type UCXBTTask_ExecuteCombatAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Cleanup();
	bAbilityFinished = false;

	AAIController* AIController = OwnerComp.GetAIOwner();

	ACXCombatCharacterBase* Character = AIController ? Cast<ACXCombatCharacterBase>(AIController->GetPawn()) : nullptr;

	if (!IsValid(Character))
	{
		return EBTNodeResult::Failed;
	}

	CombatCharacter = Character;

	UE_LOG(LogTemp, Warning, TEXT("공격 태스크 진입: %s"), *Character->GetName());

	AbilityFinishedHandle = Character->OnCombatAbilityFinished.AddUObject(this, &UCXBTTask_ExecuteCombatAbility::HandleAbilityFinished);

	if (!Character->ExecuteCombatAbility())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s : 공격 활성화 실패"), *Character->GetName());

		Cleanup();

		Character->SetDoingAction(false);
		Character->FinishAction();
		return EBTNodeResult::Succeeded;
	}

	if (bAbilityFinished)
	{
		Cleanup();
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::InProgress;

}

void UCXBTTask_ExecuteCombatAbility::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (!CombatCharacter.IsValid())
	{
		Cleanup();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (bAbilityFinished)
	{
		Cleanup();

		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UCXBTTask_ExecuteCombatAbility::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ACXCombatCharacterBase* Character = CombatCharacter.Get();

	Cleanup();

	if (IsValid(Character))
	{
		Character->CancelCombatAbility();
	}

	return EBTNodeResult::Aborted;
}

void UCXBTTask_ExecuteCombatAbility::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Cleanup();
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

void UCXBTTask_ExecuteCombatAbility::HandleAbilityFinished(bool bWasCancelled)
{
	UE_LOG(LogTemp, Warning, TEXT("공격 종료: 취소=%d"), bWasCancelled);

	// ExecuteTask 실행 도중에도 신호가 올 수 있어
	// 여기서 즉시 FinishLatentTask를 호출하지 않음.
	bAbilityFinished = true;
}

void UCXBTTask_ExecuteCombatAbility::Cleanup()
{
	if (ACXCombatCharacterBase* Character = CombatCharacter.Get())
	{
		Character->OnCombatAbilityFinished.Remove(AbilityFinishedHandle);
	}

	AbilityFinishedHandle.Reset();
	CombatCharacter.Reset();
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/AI/Combat/CXCombatAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatAIController)

ACXCombatAIController::ACXCombatAIController()
{
	static ConstructorHelpers::FObjectFinder<UBehaviorTree> BTRef(
		TEXT("/Game/CXVI/AI/Combat/BT_CombatCharacter.BT_CombatCharacter")
	);

	if (BTRef.Succeeded())
	{
		CombatBehaviorTree = BTRef.Object;
	}
}

bool ACXCombatAIController::RequestCombatAction(AActor* Target, const FVector& AttackLocation, const FVector& OriginLocation, bool bShouldApproach)
{
	UBlackboardComponent* BB = GetBlackboardComponent();

	if (!BB || !IsValid(Target))
	{
		return false;
	}

	if (BB->GetValueAsBool(TEXT("HasAction")))
	{
		return true;
	}

	BB->SetValueAsObject(TEXT("TargetActor"), Target);
	BB->SetValueAsVector(TEXT("AttackLocation"), AttackLocation);
	BB->SetValueAsVector(TEXT("OriginLocation"), OriginLocation);
	BB->SetValueAsBool(TEXT("ShouldApproach"), bShouldApproach);

	BB->SetValueAsBool(TEXT("CanReturn"), false);
	BB->SetValueAsBool(TEXT("HasAction"), true);

	return true;

}

bool ACXCombatAIController::EndCombatAction()
{
	UBlackboardComponent* BB = GetBlackboardComponent();

	if (!BB || !BB->GetValueAsBool(TEXT("HasAction")))
	{
		return false;
	}

	BB->SetValueAsBool(TEXT("HasAction"), false);

	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);

	return true;
}

bool ACXCombatAIController::RequestCombatReturn()
{
	UBlackboardComponent* BB = GetBlackboardComponent();

	if (!BB || BB->GetKeyID(TEXT("CanReturn")) == FBlackboard::InvalidKey ||
		!BB->GetValueAsBool(TEXT("HasAction")))
	{
		return false;
	}

	BB->SetValueAsBool(TEXT("CanReturn"), true);

	return true;
}

void ACXCombatAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!CombatBehaviorTree || !RunBehaviorTree(CombatBehaviorTree))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 전투 BT 실행 실패"), *GetName());
	}
}

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "CXVI/Foundation/Combat/CXCombatMessageInterface.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpecHandle.h"

#include "CXCombatCharacterBase.generated.h"

UCLASS()
class CXVI_API ACXCombatCharacterBase : public ACharacter, public IAbilitySystemInterface, public ICXCombatMessageInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ACXCombatCharacterBase();
	
	// 가스 초기화 함수
	void InitializeGAS();

	bool BasicAttackAbility();

	// 어빌리티 끝났다고 알리는 함수
	void NotifyBattleAbilityEnded(FGameplayAbilitySpecHandle Handle, bool bWasCancelled);
	void FinishAction();

	virtual void CombatMessageRequest(const FCXCombatMessageData& MessageRequest) override;
	void SubmitAction(const FCXActionSelection& Selection);
	void SubmitTarget(APawn* Target);
	void CancelSelection();


	// Getter / Setter
	FORCEINLINE const bool GetIsCharacterAlive() const { return bIsCharacterAlive; }
	FORCEINLINE const bool GetIsTurn() const { return bIsTurn; }
	FORCEINLINE void SetOriginTransform(const FTransform& NewTransform) { OriginTransform = NewTransform; }
	float GetCombatSpeed() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;

	virtual FOnTurnPhaseFinished& GetPhaseFinishedDelegate() override;

	// Inherited via IAbilitySystemInterface
	UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintImplementableEvent)
	void CXPostInitializeComponents();

	UFUNCTION()
	void OnOutOfHealthCpp(AActor* InInstigator);

	void TeleportToAttackLocation();
	void ReturnToOriginLocation();

	void ProcessMessage(const ECXCombatPhase& CurPhase);

	// 명령 실행 함수.
	void BeginTurnPhase();
	void BeforeSelectPhase();
	virtual void SelectActionPhase();
	virtual void SelectTargetPhase();
	virtual void ActionPhase();
	void ReactionPhase();
	void EndTurnPhase();

	const TArray<TWeakObjectPtr<APawn>>& GetTargetCandidates() const { return SavedMessage.Targets; }
	const FCXActionSelection& GetPendingAction() const { return PendingAction; }

	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAS")
	TObjectPtr<class UAbilitySystemComponent> ASC;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TArray<TSubclassOf<class UGameplayAbility>> Abilities;

	// 현재 실행중인 스킬
	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TArray<FGameplayAbilitySpecHandle> CurrentActivatingHandles;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "UI")
	TObjectPtr<class UWidgetComponent> HpBar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute")
	TObjectPtr<class UCXCombatAttributeSet> CharacterAttributeSet;

private:
	// 페이즈 끝남 델리게이트
	FOnTurnPhaseFinished PhaseFinishedEvent;

	// 턴매니저에 보낼 메시지
	FCXCombatMessageData SavedMessage;

	FCXActionSelection PendingAction;

	FTransform OriginTransform;
	TWeakObjectPtr<APawn> CurrentTargetPawn;

	// 어빌리티 초기화 플래그
	bool bIsInitializedAbility = false;

	bool bIsCharacterAlive = false;

	bool bIsTurn = false;


protected:
	/*UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute")
	TObjectPtr<class UBaseAttributeSet> CharacterAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute")
	TObjectPtr<class UGASCombatAttributeSet> CombatAttributeSet;*/

	//////////////////////////


};

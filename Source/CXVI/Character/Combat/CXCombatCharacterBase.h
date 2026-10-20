// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpecHandle.h"

#include "CXCombatCharacterBase.generated.h"

UCLASS()
class CXVI_API ACXCombatCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ACXCombatCharacterBase();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;

	/////////////////////////
	// ASC
public:
	// 가스 초기화 함수
	void InitializeGAS();
	// 어빌리티 끝났다고 알리는 함수
	void NotifyBattleAbilityEnded(FGameplayAbilitySpecHandle Handle, bool bWasCancelled);

protected:
	// Inherited via IAbilitySystemInterface
	UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintImplementableEvent)
	void CXPostInitializeComponents();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAS")
	TObjectPtr<class UAbilitySystemComponent> ASC;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TArray<TSubclassOf<class UGameplayAbility>> Abilities;

	// 현재 실행중인 스킬
	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TArray<FGameplayAbilitySpecHandle> CurrentActivatingHandles;

private:
	// 어빌리티 초기화 플래그
	bool bIsInitializedAbility = false;
	
	//////////////////////////
	// UI
public:
	FORCEINLINE const bool GetIsCharacterAlive() const { return bIsCharacterAlive; }

protected:
	UFUNCTION()
	void OnOutOfHealthCpp(AActor* InInstigator);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "UI")
	TObjectPtr<class UWidgetComponent> HpBar;

	bool bIsCharacterAlive = false;

	
	//////////////////////////
	// AttributeSet
public:
	float GetCombatSpeed() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute")
	TObjectPtr<class UBaseAttributeSet> CharacterAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute")
	TObjectPtr<class UGASCombatAttributeSet> CombatAttributeSet;

	//////////////////////////
	// TurnBaseInterface
public:
	//virtual void PhaseRequest(FTurnMessageResponse MessageRequest) override;

	FORCEINLINE const bool GetIsTurn() const { return bIsTurn; }

private:
	bool bIsTurn;


	//////////////////////////
	// Ability
public:
	bool ToggleAimMode();
	void BasicAttackAbility();

	FORCEINLINE const bool GetIsAimMode() const { return bIsAimMode; }
	FORCEINLINE void SetOriginTransform(const FTransform& NewTransform) { OriginTransform = NewTransform; }
	
protected:
	void TeleportToAttackLocation();
	void ReturnToOriginLocation();

protected:
	FTransform OriginTransform;
	TWeakObjectPtr<APawn> CurrentTargetPawn;

private:
	UPROPERTY(Category = "Ability_Fire")
	bool bIsAimMode = false;

};

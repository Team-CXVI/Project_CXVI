// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CXGA_AttackBase.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXGA_AttackBase : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UCXGA_AttackBase();

private:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr
	) const override;

	virtual void InputPressed(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

private:
	// 공격 몽타주가 정상적으로 끝났을 경우
	UFUNCTION()
	void OnAttackCompleted();

	// 공격 몽타주가 재생이 중단되거나 Task가 취소 됐을 경우
	UFUNCTION()
	void OnAttackInterrupted();

	UFUNCTION()
	void OnAttackEvent(FGameplayEventData Payload);

private:
	// 이벤트 태스크 등록 함수
	void RegisterEventTask();

	// 몽타주 태스크 등록 함수
	void RegisterMontageTask();

private:
	void ProcessComboCommand();

	void ComboActionBegin();

	void ComboActionEnd();

	void SetComboCheckTimer();

	void ComboCheck();



protected:
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TObjectPtr<class UAnimMontage> BasicAttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TObjectPtr<class UCXCharacterAbilityData> ComboActionData;

	UPROPERTY(VisibleAnywhere, Category = "Attack")
	uint32 CurrentCombo = 0;

	// 콤보 판정에 사용될 타이머.
	FTimerHandle ComboTimerHandle;

	UPROPERTY(VisibleAnywhere, Category = "Attack")
	bool bHasNextComboCommand = false;
	
	UPROPERTY(VisibleAnywhere, Category = "Tasks")
	TObjectPtr<class UAbilityTask_PlayMontageAndWait> MontageTask;


};

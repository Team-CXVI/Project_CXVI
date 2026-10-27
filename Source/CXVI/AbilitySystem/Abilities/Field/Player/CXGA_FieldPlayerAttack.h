// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CXGA_FieldPlayerAttack.generated.h"

/**
 * 
 */
UCLASS()
class CXVI_API UCXGA_FieldPlayerAttack : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UCXGA_FieldPlayerAttack();

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	// 애니메이션
protected:
	UFUNCTION()
	void OnAttackInterrupted()
	{
		if (IsActive())
		{
			EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, true);
		}
	}

	UFUNCTION()
	void OnAttackCompleted()
	{
		if (IsActive())
		{
			EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, false);
		}
	}

	UFUNCTION()
	void OnAttackHitEvent(FGameplayEventData Payload);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attack")
	TObjectPtr<UAnimMontage> AttackMontage;

	// 플레이어 중심에서 공격 범위의 가잠 앞쪽까지의 거리.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Trace")
	float AttackRange = 200.0f;
	
	// 공격 판정 구체의 반지름. 공격 폭은 지름에 해당.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Trace")
	float AttackRadius = 50.0f;

	// 공격 범위 그리기 디버그 설정.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Trace")
	bool bDrawAttackDebug = true;
	
};

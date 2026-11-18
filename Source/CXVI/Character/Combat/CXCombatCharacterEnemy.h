// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CXVI/Character/Combat/CXCombatCharacterBase.h"
#include "Foundation/Combat/CXCombatTypes.h"
#include "CXCombatCharacterEnemy.generated.h"

class UCXEnemyDefinition;

/**
 * 
 */
class UAnimMontage;
UCLASS()
class CXVI_API ACXCombatCharacterEnemy : public ACXCombatCharacterBase
{
	GENERATED_BODY()

public:
	ACXCombatCharacterEnemy();

	void SetInitialAttributes(const FCXInitialAttributes& InAttributes) { InitialAttributes = InAttributes; }

	void SetIsBoss(bool bInIsBoss) { bIsBoss = bInIsBoss; }
	void SetEnemyDisplayName(const FText& InDisplayName) { EnemyDisplayName = InDisplayName; }

	FORCEINLINE bool IsBoss() const { return bIsBoss; }
	FORCEINLINE const FText& GetEnemyDisplayName() const { return EnemyDisplayName; }
	FORCEINLINE void SetInitialSkills(const TArray<FCXEnemySkillSpawnEntry>& InSkills){ InitialSkills = InSkills;}

protected:
	virtual void PostInitializeComponents() override;

	virtual void SelectActionPhase() override;
	virtual void SelectTargetPhase() override;

	virtual const FGameplayAbilitySpec* GetSelectedAttackSpec() const override;

	// 스킬 행으로 받은 어빌리티면 Skill, 아니면(기본 공격 태그로 고른 경우) BasicAttack
	virtual ECXActionType GetSelectedActionType() const override;
	virtual FText GetSelectedActionName() const override;

	virtual bool ExecuteCombatAbility() override;

private:
	void GrantInitialSkills();



protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute")
	TObjectPtr<class UCXCombatEnemyAttributeSet> AttributeSet;

	virtual void StartDodgeMontage() override;
	virtual void StartHitReactMontage() override;
	virtual void PlayDeathPresentation() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> DodgeMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimMontage> HitReactMontage;

	

private:
	FCXInitialAttributes InitialAttributes;

	// 보스는 머리 위 HpBar 대신 전투 HUD의 보스 HP UI로 표시
	bool bIsBoss = false;

	FText EnemyDisplayName;

	TArray<FCXEnemySkillSpawnEntry> InitialSkills;
	TMap<FGameplayAbilitySpecHandle, int32> SkillWeights;

	// 스킬 행의 DisplayName (GrantInitialSkills에서 채움)
	TMap<FGameplayAbilitySpecHandle, FText> SkillDisplayNames;

	
};

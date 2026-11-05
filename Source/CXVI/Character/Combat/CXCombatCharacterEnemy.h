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
UCLASS()
class CXVI_API ACXCombatCharacterEnemy : public ACXCombatCharacterBase
{
	GENERATED_BODY()

public:
	ACXCombatCharacterEnemy();

	void SetInitialAttributes(const FCXInitialAttributes& InAttributes) { InitialAttributes = InAttributes; }

protected:
	virtual void PostInitializeComponents() override;

	virtual void SelectTargetPhase() override;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute")
	TObjectPtr<class UCXCombatEnemyAttributeSet> AttributeSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Focus")
	TObjectPtr<class USceneComponent> EnemyTargetFocusPoint;

private:
	FCXInitialAttributes InitialAttributes;

	
};

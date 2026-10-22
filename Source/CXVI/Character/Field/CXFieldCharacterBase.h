// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayAbilitySpec.h"


#include "CXFieldCharacterBase.generated.h"

class UCXBaseAttributeSet;
class UAbilitySystemComponent;
class UGameplayAbility;
class UGameplayEffect;
class UAnimMontage;
struct FGameplayEventData;

UCLASS()
class CXVI_API ACXFieldCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	ACXFieldCharacterBase();

	virtual void PossessedBy(AController* NewController) override;

	// 필드 캐릭터의 ASC를 직접 조회한다.
	UAbilitySystemComponent* GetAbilitySystemComponent() const;


protected:
	virtual void BeginPlay() override;

	virtual void PostInitializeComponents() override;


protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> ASC;
};

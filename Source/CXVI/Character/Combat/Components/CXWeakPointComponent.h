// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "CXWeakPointComponent.generated.h"

class UGameplayEffect;
class UCXWeakPointComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCXWeakPointBroken, UCXWeakPointComponent*, WeakPoint);

/**
 * 
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class CXVI_API UCXWeakPointComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	UCXWeakPointComponent();

	FORCEINLINE bool IsBroken() const {	return bBroken; }
	FORCEINLINE float GetDamageMultiplier() const { return bBroken ? 1.f : FMath::Max(1.f, DamageMultiplier); }
	FORCEINLINE const TArray<TSubclassOf<UGameplayEffect>>& GetBreakEffects() const { return BreakEffects; }

	// 처음 파괴 될때만 true
	bool TryBreak();

public:
	// 약점 파괴시 표시 이벤트 들을 연결할 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Combat|WeakPoint")
	FCXWeakPointBroken OnWeakPointBroken;

private:
	UPROPERTY(EditAnywhere, Category = "Combat|WeakPoint")
	FName WeakPointId = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Combat|WeakPoint", meta = (ClampMin = "1.0"))
	float DamageMultiplier = 3.f;

	// 이후 파괴 처리에서 적용할 그로기 지속 피해 GE목록
	UPROPERTY(EditAnywhere, Category = "Combat|WeakPoint")
	TArray<TSubclassOf<UGameplayEffect>> BreakEffects;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|WeakPoint", meta = (AllowPrivateAccess = "true"))
	bool bBroken = false;
		
};

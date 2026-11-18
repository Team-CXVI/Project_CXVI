// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "AttributeSet.h"

#include "CXCombatantViewModel.generated.h"

class UTexture2D;

/**
 * 전투원 한 명의 ViewModel (파티 패널 한 칸, 보스 HP UI)
 * - Bind : 전투원 ASC의 속성 변경 델리게이트를 구독해서 값 갱신 (전투 코드는 읽기만 함)
 * - Unbind : 구독 해제. 전투원 Destroy 전에 호출해야 함
 * - Portrait : UCXCombatViewModel이 생성 시 넣음. 위젯이 Soft 참조를 비동기 로드 (WBP에서 Portrait → SetPortrait 바인딩)
 */
UCLASS(BlueprintType)
class CXVI_API UCXCombatantViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()
	
public:
	void Bind(UAbilitySystemComponent* InASC);

	void Unbind();

	void SetDisplayName(const FText& InDisplayName);

	void SetPortrait(const TSoftObjectPtr<UTexture2D>& InPortrait);

	float GetHealth() const { return Health; }
	float GetMaxHealth() const { return MaxHealth; }
	float GetAP() const { return AP; }
	float GetMaxAP() const { return MaxAP; }

	// "현재 / 최대" (HP 수치 TextBlock 바인딩용)
	UFUNCTION(BlueprintPure, FieldNotify, Category = "CXVI|Combat")
	FText GetHealthText() const;

	// 0 ~ 1 (ProgressBar.Percent 바인딩용)
	UFUNCTION(BlueprintPure, FieldNotify, Category = "CXVI|Combat")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, FieldNotify, Category = "CXVI|Combat")
	bool IsDead() const;

private:
	// 현재 값으로 초기화한 뒤 변경 구독
	void BindAttribute(const FGameplayAttribute& InAttribute, void (ThisClass::* InSetter)(float));

	void SetHealth(float InValue);
	void SetMaxHealth(float InValue);
	void SetAP(float InValue);
	void SetMaxAP(float InValue);

private:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CXVI|Combat", meta = (AllowPrivateAccess = true))
	float Health = 0.f;
	
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CXVI|Combat", meta = (AllowPrivateAccess = true))
	float MaxHealth = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CXVI|Combat", meta = (AllowPrivateAccess = true))
	float AP = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CXVI|Combat", meta = (AllowPrivateAccess = true))
	float MaxAP = 0.f;

	// 보스 HP UI 이름 표시용. 파티 칸에서는 채우지 않음
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CXVI|Combat", meta = (AllowPrivateAccess = true))
	FText DisplayName;

	// 파티 패널 초상화용. 비어 있을 수 있음
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CXVI|Combat", meta = (AllowPrivateAccess = true))
	TSoftObjectPtr<UTexture2D> Portrait;

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	TArray<TPair<FGameplayAttribute, FDelegateHandle>> AttributeHandles;
};

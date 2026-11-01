// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"

#include "CXPartyMemberViewModel.generated.h"

struct FCXPartyMemberState;
class UCXCharacterDefinition;
class UTexture2D;

/**
 * 파티원 한 명의 ViewModel (필드 HUD 파티원 상태 한 칸)
 * - 유저 데이터(FCXPartyMemberState) + 캐릭터 정의 기준의 표시 값
 * - 값은 UCXPartyViewModel이 Refresh로 넣음 (이 클래스는 데이터 출처를 모름)
 */
UCLASS(BlueprintType)
class CXVI_API UCXPartyMemberViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()
	
public:
	void Refresh(const FCXPartyMemberState& InMember, const UCXCharacterDefinition* InDefinition, float InMaxHealth);

	// "현재 / 최대" (HP 수치 TextBlock 바인딩용)
	UFUNCTION(BlueprintPure, FieldNotify, Category = "CXVI|Party")
	FText GetHealthText() const;

	// 0 ~ 1 (ProgressBar.Percent 바인딩용)
	UFUNCTION(BlueprintPure, FieldNotify, Category = "CXVI|Party")
	float GetHealthPercent() const;

private:
	void SetPortrait(const TSoftObjectPtr<UTexture2D>& InValue);
	void SetLevel(int32 InValue);
	void SetHealth(float InValue);
	void SetMaxHealth(float InValue);

private:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CXVI|Party", meta = (AllowPrivateAccess = true))
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CXVI|Party", meta = (AllowPrivateAccess = true))
	int32 Level = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CXVI|Party", meta = (AllowPrivateAccess = true))
	float Health = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CXVI|Party", meta = (AllowPrivateAccess = true))
	float MaxHealth = 0.f;

};

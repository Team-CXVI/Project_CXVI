// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "CXCombatViewModel.generated.h"

class ACXCombatCharacterBase;
class UCXCombatantViewModel;

/**
 * 전투 HUD 전체 표시 데이터
 * - 전투 Scene이 생성 / 소유하고 HUD에 주입 (WBP Viewmodel 생성 방식 = Manual)
 * - SetCombatants : 아레나 전투원으로 파티 / 보스 ViewModel 구성
 * - Reset : 전체 구독 해제. 전투원 정리(CleanupCombatants) 전에 호출
 * - 일반 몬스터 HP는 전투 캐릭터의 WidgetComponent(전투 파트) 담당 → 여기서는 Character.Type.Boss 태그가 있는 적만
 */
UCLASS(BlueprintType)
class CXVI_API UCXCombatViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()
	
public:
	// 여기서 mvvm으로 표시하는 보스 몬스터는 1마리
	void SetCombatants(const TArray<TObjectPtr<ACXCombatCharacterBase>>& InPlayers, const ACXCombatCharacterBase* InEnemy);

	void Reset();

	UFUNCTION(BlueprintPure, FieldNotify, Category = "CXVI|Combat")
	bool HasBoss() const { return nullptr != Boss; }

private:
	UCXCombatantViewModel* CreateCombatantViewModel(const AActor* InCombatant);

	void BroadcastAll();

private:
	// 파티 패널 (Set Items 바인딩)
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CXVI|Combat", meta = (AllowPrivateAccess = true))
	TArray<TObjectPtr<UCXCombatantViewModel>> Players;

	// 보스 HP UI (보스가 없으면 nullptr)
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CXVI|Combat", meta = (AllowPrivateAccess = true))
	TObjectPtr<UCXCombatantViewModel> Boss;
	
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"

#include "CXDamageNumberComponent.generated.h"

class UWidgetComponent;
class UCXDamageNumberStackWidget;
struct FCXDamageResultMessage;

/**
 * 전투 데미지 숫자 표시 관리 (ACXCombatArena에 1개 부착)
 * - Message.Combat.DamageResult를 받아 대상의 컨테이너(UCXDamageNumberStackWidget)에 결과 추가
 * - 컨테이너 = 대상의 기준점(AnchorTag 컴포넌트 태그)에 붙인 Screen 공간 UWidgetComponent. 대상당 1개, 처음 받을 때 생성 후 재사용
 * - 기준점이 없는 대상은 표시하지 않고 경고 로그
 * - 구독 기간은 BeginPlay ~ EndPlay (서브레벨 Hide / Show 때 다시 호출됨)
 */
UCLASS(ClassGroup = (UI), meta = (BlueprintSpawnableComponent))
class CXVI_API UCXDamageNumberComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCXDamageNumberComponent();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleDamageResult(const FGameplayTag& InChannel, const FCXDamageResultMessage& InMessage);

	UCXDamageNumberStackWidget* FindOrCreateStack(AActor* InTarget);

	void ClearStacks();

private:
	UPROPERTY(EditAnywhere, Category = "DamageNumber")
	TSubclassOf<UCXDamageNumberStackWidget> StackWidgetClass;

	// 캐릭터 BP의 기준점 SceneComponent에 붙인 Component Tags 이름
	UPROPERTY(EditAnywhere, Category = "DamageNumber")
	FName AnchorTag = TEXT("DamageNumberAnchor");

	// 대상별 컨테이너
	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<UWidgetComponent>> Stacks;

	FCXGameplayMessageListenerHandle DamageResultListener;
};
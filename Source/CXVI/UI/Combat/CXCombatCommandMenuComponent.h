// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "Foundation/Combat/CXCombatTypes.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"

#include "CXCombatCommandMenuComponent.generated.h"

struct FCXCombatCommandMenuMessage;

/**
 * 전투 Command Menu를 3D 공간에 표시하는 위젯 컴포넌트 (ACXCombatArena에 1개 부착)
 * - World 공간에 그려져 메뉴 앞에 있는 캐릭터가 깊이 비교로 메뉴를 가림
 * - Message.Combat.CommandMenuChanged를 받아 위치, 표시 여부, 위젯 내용을 갱신
 * - 위치 = 턴 캐릭터의 OriginTransform 기준 + MenuPlacements의 메뉴 상태별 배치값
 * - 구독 기간은 BeginPlay ~ EndPlay. 숨겨진 위젯의 Construct 시점에 의존하지 않도록 위젯이 아닌 컴포넌트에서 구독
 * - 메뉴 갱신 시 패널 애니메이션 첫 상태를 바로 적용하고 렌더 타깃을 즉시 다시 그림 (이전 화면이 먼저 보이는 문제 방지)
 */
UCLASS()
class CXVI_API UCXCombatCommandMenuComponent : public UWidgetComponent
{
	GENERATED_BODY()
	
public:
	UCXCombatCommandMenuComponent();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleCommandMenuChanged(const FGameplayTag& InChannel, const FCXCombatCommandMenuMessage& InMessage);
	
private:
	// 턴 캐릭터 OriginTransform 기준 배치값. 항목이 없는 메뉴 상태에서는 메뉴를 숨김
	UPROPERTY(EditAnywhere, Category = "CommandMenu")
	TMap<ECommandMenuState, FTransform> MenuPlacements;

	FCXGameplayMessageListenerHandle CommandMenuListener;

	FTransform InitTransform;
};

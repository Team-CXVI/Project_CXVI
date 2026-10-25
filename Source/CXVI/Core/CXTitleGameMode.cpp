// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/CXTitleGameMode.h"

#include "CXPlayerController.h"
#include "UI/CXUIManagerSubsystem.h"
#include "UI/CXActivatableWidget.h"
#include "Tags/CXGameplayTags.h"
#include "Level/CXLevelTransitionSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXTitleGameMode)

ACXTitleGameMode::ACXTitleGameMode()
{
	// UI 레이아웃은 ACXPlayerController가 생성하므로 타이틀에서도 같은 PC 사용
	PlayerControllerClass = ACXPlayerController::StaticClass();
}

void ACXTitleGameMode::BeginPlay()
{
	Super::BeginPlay();

	// PlayerController의 ReceivedPlayer에서 루트 레이아웃을 생성
	// ReceivedPlayer가 GameMode의 BeginPlay보다 먼저 호출되므로 여기서 위젯을 추가
	if (TitleScreenClass)
	{
		if (UCXUIManagerSubsystem* UIManager = UCXUIManagerSubsystem::TryGet(this))
		{
			UIManager->PushWidgetToLayer<UCXActivatableWidget>(CXVIGameplayTags::UI_Layer_Menu, TitleScreenClass);
		}
	}

	if (UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this))
	{	
		// 타이틀의 경우 UI가 준비되면 레벨 준비가 끝난 상태이므로 여기서 레벨 전이 완료 상태를 알림
		LevelTransition->NotifyLevelReady();
	}
}

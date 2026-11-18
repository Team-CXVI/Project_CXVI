// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/CXPlayerController.h"

#include "UI/CXUIManagerSubsystem.h"
#include "Level/CXLevelTransitionSubsystem.h"
#include <Engine/LocalPlayer.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXPlayerController)

void ACXPlayerController::PostInitializeComponents()
{
	// 여기서 PlayerCameraManager가 생성됨
	Super::PostInitializeComponents();

	// 새 월드의 첫 프레임부터 화면을 가린 상태로 시작 (레벨 전환 중일 때만)
	if (const UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this))
	{
		LevelTransition->ApplyScreenCover(PlayerCameraManager);
	}
}

void ACXPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UCXUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UCXUIManagerSubsystem>())
		{
			UIManager->CreatePrimaryLayout(this);
		}
	}
}

void ACXPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UCXUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UCXUIManagerSubsystem>())
		{
			// 이 PC가 만든 레이아웃일 때만 제거 (새 맵의 레이아웃을 지우지 않도록)
			const UCXPrimaryGameLayout* Layout = UIManager->GetPrimaryLayout();
			if (Layout && Layout->GetOwningPlayer() == this)
			{
				UIManager->DestroyPrimaryLayout();
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

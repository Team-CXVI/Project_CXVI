// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/CXPlayerController.h"

#include "UI/CXUIManagerSubsystem.h"
#include <Engine/LocalPlayer.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXPlayerController)

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

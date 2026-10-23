// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXUIManagerSubsystem.h"

#include "CXUISettings.h"

#include <CommonActivatableWidget.h>
#include <Engine/LocalPlayer.h>
#include <GameFramework/PlayerController.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXUIManagerSubsystem)

DEFINE_LOG_CATEGORY(LogCXUI);

UCXUIManagerSubsystem& UCXUIManagerSubsystem::Get(const UObject* WorldContextObject)
{
	UCXUIManagerSubsystem* Subsystem = TryGet(WorldContextObject);
	check(Subsystem);
	return *Subsystem;
}

UCXUIManagerSubsystem* UCXUIManagerSubsystem::TryGet(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	const ULocalPlayer* LocalPlayer = World ? World->GetFirstLocalPlayerFromController() : nullptr;
	return LocalPlayer ? LocalPlayer->GetSubsystem<UCXUIManagerSubsystem>() : nullptr;
}

void UCXUIManagerSubsystem::Deinitialize()
{
	DestroyPrimaryLayout();

	Super::Deinitialize();
}

void UCXUIManagerSubsystem::CreatePrimaryLayout(APlayerController* InPlayerController)
{
	if (nullptr == InPlayerController || !InPlayerController->IsLocalController())
	{
		return;
	}

	// 월드 이동으로 이전 레이아웃이 남아 있을 수 있으므로 먼저 정리
	DestroyPrimaryLayout();

	// DefaultGame.ini에서 Layout UI 경로를 획득
	const UCXUISettings* Settings = GetDefault<UCXUISettings>();
	const TSubclassOf<UCXPrimaryGameLayout> LayoutClass = Settings->PrimaryLayoutClass.LoadSynchronous();
	// CLASS_Abstract -> 추상 클래스인 경우. UCXPrimaryGameLayout를 있는 그대로 사용한 경우
	if (nullptr == LayoutClass || LayoutClass->HasAnyClassFlags(CLASS_Abstract))
	{
		UE_LOG(LogCXUI, Error, TEXT("Primary Layout Class가 없거나 Abstract입니다. (Project Settings > Game > CXVI UI)"));
		return;
	}

	// CommonUI 입력 처리에 LocalPlayer가 필요하므로 Owner를 PlayerController로 지정
	PrimaryLayout = CreateWidget<UCXPrimaryGameLayout>(InPlayerController, LayoutClass);
	if (nullptr == PrimaryLayout)
	{
		return;
	}

	// 이건 필요 없는 것 같긴한데 그래도 한번 더
	PrimaryLayout->SetPlayerContext(FLocalPlayerContext(GetLocalPlayer<ULocalPlayer>()));
	PrimaryLayout->AddToPlayerScreen(Settings->LayoutZOrder);

	// IMC 바인딩
	ApplyUIInputMapping(true);

	UE_LOG(LogCXUI, Log, TEXT("Primary layout created: %s"), *GetNameSafe(PrimaryLayout));
}

void UCXUIManagerSubsystem::DestroyPrimaryLayout()
{

}

UCommonActivatableWidget* UCXUIManagerSubsystem::K2_PushWidgetToLayer(FGameplayTag InLayerTag, TSubclassOf<UCommonActivatableWidget> InWidgetClass)
{
	return PushWidgetToLayer<UCommonActivatableWidget>(InLayerTag, InWidgetClass);
}

void UCXUIManagerSubsystem::RemoveWidget(UCommonActivatableWidget* InWidget)
{

}

void UCXUIManagerSubsystem::ApplyUIInputMapping(bool bInApply) const
{
}

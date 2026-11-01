// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXUIManagerSubsystem.h"

#include "CXUISettings.h"
#include "CXNavigationConfig.h"
#include "UI/ViewModel/CXPartyViewModel.h"
#include "UI/ViewModel/CXPotionViewModel.h"

#include <CommonActivatableWidget.h>
#include <Engine/LocalPlayer.h>
#include <GameFramework/PlayerController.h>
#include <EnhancedInputSubsystems.h>
#include <InputMappingContext.h>
#include <Input/CommonUIActionRouterBase.h>
#include <GameFramework/InputSettings.h>
#include <MVVMGameSubsystem.h>

#if WITH_EDITOR
#include <Engine/GameViewportClient.h>
#endif

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

void UCXUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// 새 내비게이션 설정
	ApplyNavigationConfig(true);

	// 위젯이 생성되기 전에 전역 Viewmodel Collection에 등록
	CreatePartyViewModel();

#if WITH_EDITOR
	if (FSlateApplication::IsInitialized())
	{
		// 에디터 실행 후 포커싱이 들어왔을 때 실행할 델리게이트 바운딩
		FocusChangingHandle = FSlateApplication::Get().OnFocusChanging().AddUObject(this, &ThisClass::HandleSlateFocusChanging);
	}
#endif
}

void UCXUIManagerSubsystem::Deinitialize()
{
	DestroyPrimaryLayout();

	DestroyPartyViewModel();

	ApplyNavigationConfig(false);

#if WITH_EDITOR
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().OnFocusChanging().Remove(FocusChangingHandle);
	}
#endif

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

	// 이전 레벨 화면의 입력 설정이 남지 않도록 초기화 (이 레벨의 화면이 활성화되면 그 설정으로 덮어씀)
	ResetInputConfigToGameDefault();

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
	if (nullptr == PrimaryLayout)
	{
		return;
	}

	// IMC 바인딩 해제
	ApplyUIInputMapping(false);

	PrimaryLayout->RemoveFromParent();
	PrimaryLayout = nullptr;
}

UCommonActivatableWidget* UCXUIManagerSubsystem::K2_PushWidgetToLayer(FGameplayTag InLayerTag, TSubclassOf<UCommonActivatableWidget> InWidgetClass)
{
	return PushWidgetToLayer<UCommonActivatableWidget>(InLayerTag, InWidgetClass);
}

void UCXUIManagerSubsystem::RemoveWidget(UCommonActivatableWidget* InWidget)
{
	if (PrimaryLayout)
	{
		PrimaryLayout->FindAndRemoveWidgetFromLayer(InWidget);
	}
}

void UCXUIManagerSubsystem::ApplyUIInputMapping(bool bInApply) const
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (nullptr == InputSubsystem)
	{
		return;
	}

	const UCXUISettings* Settings = GetDefault<UCXUISettings>();
	UInputMappingContext* MappingContext = Settings->UIMappingContext.LoadSynchronous();
	if (nullptr == MappingContext)
	{
		UE_LOG(LogCXUI, Warning, TEXT("UI Mapping Context가 지정되지 않았습니다. (Project Settings > Game > CXVI UI)"));
		return;
	}

	// 키 리바인딩(EnhancedInputUserSettings)에 IMC 등록
	FModifyContextOptions Options;
	Options.bNotifyUserSettings = true;

	if (bInApply)
	{
		InputSubsystem->AddMappingContext(MappingContext, Settings->UIMappingPriority, Options);
	}
	else
	{
		InputSubsystem->RemoveMappingContext(MappingContext, Options);
	}
}

void UCXUIManagerSubsystem::ApplyNavigationConfig(bool bInApply)
{
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}

	FSlateApplication& SlateApplication = FSlateApplication::Get();

	if (bInApply)
	{
		// 싱글 플레이 기준: 이미 등록했으면 다시 등록하지 않음 (복구할 원래 설정 보존)
		if (!PreviousNavigationConfig.IsValid())
		{
			PreviousNavigationConfig = SlateApplication.GetNavigationConfig();
			SlateApplication.SetNavigationConfig(MakeShared<FCXNavigationConfig>());
		}
	}
	else if (PreviousNavigationConfig.IsValid())
	{
		SlateApplication.SetNavigationConfig(PreviousNavigationConfig.ToSharedRef());
		PreviousNavigationConfig.Reset();
	}
}

void UCXUIManagerSubsystem::ResetInputConfigToGameDefault() const
{
	UCommonUIActionRouterBase* ActionRouter = GetLocalPlayer() ? GetLocalPlayer()->GetSubsystem<UCommonUIActionRouterBase>() : nullptr;
	if (nullptr == ActionRouter)
	{
		return;
	}

	// 마우스 캡처는 프로젝트 기본값(Project Settings > Input > Default Viewport Mouse Capture Mode)을 따름
	const EMouseCaptureMode CaptureMode = GetDefault<UInputSettings>()->DefaultViewportMouseCaptureMode;
	ActionRouter->SetActiveUIInputConfig(FUIInputConfig(ECommonInputMode::Game, CaptureMode));
}

void UCXUIManagerSubsystem::CreatePartyViewModel()
{
	// LocalPlayer Subsystem의 Initialize 시점에는 월드가 없을 수 있으므로 GameInstance에서 직접 가져옴
	UGameInstance* GameInstance = GetLocalPlayer() ? GetLocalPlayer()->GetGameInstance() : nullptr;
	const UMVVMGameSubsystem* MVVMSubsystem = GameInstance ? GameInstance->GetSubsystem<UMVVMGameSubsystem>() : nullptr;
	UMVVMViewModelCollectionObject* Collection = MVVMSubsystem ? MVVMSubsystem->GetViewModelCollection() : nullptr;
	if (nullptr == Collection)
	{
		UE_LOG(LogCXUI, Error, TEXT("MVVM 전역 Viewmodel Collection이 없어 PartyViewModel을 등록하지 못했습니다."));
		return;
	}

	UCXPartyViewModel* PartyViewModel = NewObject<UCXPartyViewModel>(GameInstance);
	PartyViewModel->Initialize(*GameInstance);

	if (false == Collection->AddViewModelInstance(UCXPartyViewModel::MakeGlobalContext(), PartyViewModel))
	{
		UE_LOG(LogCXUI, Error, TEXT("PartyViewModel을 전역 Viewmodel Collection에 등록하지 못했습니다."));
		PartyViewModel->Reset();
	}
}

void UCXUIManagerSubsystem::DestroyPartyViewModel()
{
	const UGameInstance* GameInstance = GetLocalPlayer() ? GetLocalPlayer()->GetGameInstance() : nullptr;
	const UMVVMGameSubsystem* MVVMSubsystem = GameInstance ? GameInstance->GetSubsystem<UMVVMGameSubsystem>() : nullptr;
	UMVVMViewModelCollectionObject* Collection = MVVMSubsystem ? MVVMSubsystem->GetViewModelCollection() : nullptr;
	if (nullptr == Collection)
	{
		return;
	}

	UCXPartyViewModel* PartyViewModel = Cast<UCXPartyViewModel>(Collection->FindFirstViewModelInstanceOfType(UCXPartyViewModel::StaticClass()));
	if (nullptr == PartyViewModel)
	{
		return;
	}

	// 구독 해제 후 Collection에서 제거 (Collection의 참조가 사라지면 GC 대상)
	PartyViewModel->Reset();
	Collection->RemoveAllViewModelInstance(PartyViewModel);
}

void UCXUIManagerSubsystem::CreatePotionViewModel()
{
	// LocalPlayer Subsystem의 Initialize 시점에는 월드가 없을 수 있으므로 GameInstance에서 직접 가져옴
	UGameInstance* GameInstance = GetLocalPlayer() ? GetLocalPlayer()->GetGameInstance() : nullptr;
	const UMVVMGameSubsystem* MVVMSubsystem = GameInstance ? GameInstance->GetSubsystem<UMVVMGameSubsystem>() : nullptr;
	UMVVMViewModelCollectionObject* Collection = MVVMSubsystem ? MVVMSubsystem->GetViewModelCollection() : nullptr;
	if (nullptr == Collection)
	{
		UE_LOG(LogCXUI, Error, TEXT("MVVM 전역 Viewmodel Collection이 없어 PotionViewModel을 등록하지 못했습니다."));
		return;
	}

	UCXPotionViewModel* PotionViewModel = NewObject<UCXPotionViewModel>(GameInstance);
	PotionViewModel->Initialize(*GameInstance);

	if (false == Collection->AddViewModelInstance(UCXPotionViewModel::MakeGlobalContext(), PotionViewModel))
	{
		UE_LOG(LogCXUI, Error, TEXT("PotionViewModel을 전역 Viewmodel Collection에 등록하지 못했습니다."));
		PotionViewModel->Reset();
	}
}

void UCXUIManagerSubsystem::DestroyPotionViewModel()
{
	const UGameInstance* GameInstance = GetLocalPlayer() ? GetLocalPlayer()->GetGameInstance() : nullptr;
	const UMVVMGameSubsystem* MVVMSubsystem = GameInstance ? GameInstance->GetSubsystem<UMVVMGameSubsystem>() : nullptr;
	UMVVMViewModelCollectionObject* Collection = MVVMSubsystem ? MVVMSubsystem->GetViewModelCollection() : nullptr;
	if (nullptr == Collection)
	{
		return;
	}

	UCXPotionViewModel* PotionViewModel = Cast<UCXPotionViewModel>(Collection->FindFirstViewModelInstanceOfType(UCXPotionViewModel::StaticClass()));
	if (nullptr == PotionViewModel)
	{
		return;
	}

	// 구독 해제 후 Collection에서 제거 (Collection의 참조가 사라지면 GC 대상)
	PotionViewModel->Reset();
	Collection->RemoveAllViewModelInstance(PotionViewModel);
}

#if WITH_EDITOR

void UCXUIManagerSubsystem::HandleSlateFocusChanging(const FFocusEvent& InFocusEvent, const FWeakWidgetPath& InOldFocusedPath, const TSharedPtr<SWidget>& InOldFocusedWidget, const FWidgetPath& InNewFocusedPath, const TSharedPtr<SWidget>& InNewFocusedWidget)
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (nullptr == PrimaryLayout || nullptr == LocalPlayer || nullptr == LocalPlayer->ViewportClient)
	{
		return;
	}

	const TSharedPtr<SViewport> ViewportWidget = LocalPlayer->ViewportClient->GetGameViewportWidget();
	if (!ViewportWidget.IsValid())
	{
		return;
	}

	// 뷰포트 밖이 포커스인 경우 처리하지 않음
	const bool bWasInViewport = InOldFocusedPath.IsValid() && InOldFocusedPath.ContainsWidget(ViewportWidget.Get());
	const bool bIsInViewport = InNewFocusedPath.ContainsWidget(ViewportWidget.Get());
	if (bWasInViewport || !bIsInViewport)
	{
		return;
	}

	// 포커스 변경 처리 도중에 다시 포커스를 바꾸지 않도록 다음 틱에 적용
	if (UWorld* World = PrimaryLayout->GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::ReapplyTopWidgetInputConfig);
	}
}

void UCXUIManagerSubsystem::ReapplyTopWidgetInputConfig()
{
	UCommonActivatableWidget* TopWidget = PrimaryLayout ? PrimaryLayout->GetTopActiveWidget() : nullptr;
	UCommonUIActionRouterBase* ActionRouter = TopWidget ? UCommonUIActionRouterBase::Get(*TopWidget) : nullptr;
	if (nullptr == ActionRouter)
	{
		return;
	}

	// 입력 설정을 선언하지 않는 화면은 건드리지 않음
	const TOptional<FUIInputConfig> DesiredConfig = TopWidget->GetDesiredInputConfig();
	if (!DesiredConfig.IsSet())
	{
		return;
	}

	// 엔진이 이미 적용한 경우는 처리하지 않음 (활성화 시점에 건너뛴 경우만 보완)
	const bool bAlreadyApplied = ActionRouter->GetActiveInputMode() == DesiredConfig->GetInputMode()
		&& ActionRouter->GetActiveMouseCaptureMode() == DesiredConfig->GetMouseCaptureMode();
	if (bAlreadyApplied)
	{
		return;
	}

	ActionRouter->SetActiveUIInputConfig(DesiredConfig.GetValue(), TopWidget);
	ActionRouter->RefreshActiveRootFocus();
}

#endif // WITH_EDITOR


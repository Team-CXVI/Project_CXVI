// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXHoldActionWidget.h"

#include "UI/CXInputIconWidget.h"
#include "UI/CXUIManagerSubsystem.h"
#include "UI/CXUISettings.h"

#include <Components/Image.h>
#include <Components/TextBlock.h>
#include <Input/CommonUIInputTypes.h>
#include <InputAction.h>
#include <InputMappingContext.h>
#include <InputTriggers.h>
#include <Materials/MaterialInstanceDynamic.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXHoldActionWidget)

namespace CXHoldAction
{
	// IA의 Hold 트리거를 CommonUI Hold로 인식
	static const TCHAR* ResolveHoldKeysCVarName = TEXT("CommonUI.bEnhancedInputDynamicallyResolveHoldKeysForInputAction");

	// 짧은 탭을 일반 Press로 바꾸지 않음
	static const TCHAR* WaitForHoldCompleteCVarName = TEXT("CommonUI.AlwaysWaitForHoldActionToComplete");

	static bool IsConsoleVariableEnabled(const TCHAR* InName)
	{
		const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(InName);
		return CVar && CVar->GetBool();
	}
}

void UCXHoldActionWidget::SetHoldActionActive(bool bInActive)
{
	if (bHoldActionActive == bInActive)
	{
		return;
	}

	bHoldActionActive = bInActive;

	SetGaugePercent(0.f);

	if (bHoldActionActive)
	{
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		RegisterHoldBinding();
	}
	else
	{
		UnregisterHoldBinding();
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCXHoldActionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Brush에 지정한 머티리얼로 Dynamic Material Instance를 한 번 만들어 Brush에 적용 (이후 파라미터만 갱신)
	HoldGaugeMaterial = HoldGaugeImage ? HoldGaugeImage->GetDynamicMaterial() : nullptr;
	if (nullptr == HoldGaugeMaterial)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] HoldGaugeImage의 Brush에 머티리얼이 지정되지 않아 게이지를 표시하지 않습니다."), *GetName());
	}

	SetGaugePercent(0.f);

	if (nullptr == HoldAction)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] HoldAction이 지정되지 않았습니다."), *GetName());
	}
	else
	{
		bHoldInputConfigured = IsHoldInputConfigured();
		RefreshKeyIcon();
	}

	// SetHoldActionActive(true) 전까지 표시하지 않음
	SetVisibility(ESlateVisibility::Collapsed);
}

void UCXHoldActionWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 여기서 초기화 해야 디자이너 창에서도 반영됨
	if (LabelText)
	{
		LabelText->SetText(Label);
	}
}

void UCXHoldActionWidget::NativeDestruct()
{
	SetHoldActionActive(false);

	Super::NativeDestruct();
}

bool UCXHoldActionWidget::IsHoldInputConfigured() const
{
	bool bConfigured = true;

	const bool bHasHoldTrigger = HoldAction->Triggers.ContainsByPredicate([](const TObjectPtr<UInputTrigger>& InTrigger)
		{
			const UInputTriggerHold* HoldTrigger = Cast<UInputTriggerHold>(InTrigger);
			return HoldTrigger && HoldTrigger->HoldTimeThreshold > 0.f;
		});

	if (false == bHasHoldTrigger)
	{
		UE_LOG(LogCXUI, Error, TEXT("[%s] %s에 Hold 트리거(Hold Time Threshold > 0)가 없어 입력을 바인딩하지 않습니다."), *GetName(), *HoldAction->GetName());
		bConfigured = false;
	}

	for (const TCHAR* CVarName : { CXHoldAction::ResolveHoldKeysCVarName, CXHoldAction::WaitForHoldCompleteCVarName })
	{
		if (false == CXHoldAction::IsConsoleVariableEnabled(CVarName))
		{
			UE_LOG(LogCXUI, Error, TEXT("[%s] %s=1 이 아니어서 입력을 바인딩하지 않습니다. (Config/DefaultEngine.ini [ConsoleVariables])"), *GetName(), CVarName);
			bConfigured = false;
		}
	}

	return bConfigured;
}

void UCXHoldActionWidget::RegisterHoldBinding()
{
	if (false == bHoldInputConfigured)
	{
		return;
	}

	FBindUIActionArgs BindArgs(HoldAction, /*bShouldDisplayInActionBar*/ false, FSimpleDelegate::CreateUObject(this, &ThisClass::OnHoldCompleted));
	BindArgs.InputMode = ECommonInputMode::Game;
	BindArgs.OnHoldActionProgressed.BindUObject(this, &ThisClass::SetGaugePercent);

	HoldBinding = RegisterUIActionBinding(BindArgs);
}

void UCXHoldActionWidget::UnregisterHoldBinding()
{
	if (false == HoldBinding.IsValid())
	{
		return;
	}

	// RemoveActionBinding : 위젯과 입력 처리 목록에서 제거 / Unregister : 바인딩 자체를 삭제
	RemoveActionBinding(HoldBinding);
	HoldBinding.Unregister();
	HoldBinding = FUIActionBindingHandle();
}

void UCXHoldActionWidget::SetGaugePercent(float InPercent)
{
	if (HoldGaugeMaterial)
	{
		HoldGaugeMaterial->SetScalarParameterValue(GaugePercentParamName, InPercent);
	}
}

void UCXHoldActionWidget::RefreshKeyIcon()
{
	if (nullptr == KeyIcon)
	{
		return;
	}

	const UInputMappingContext* MappingContext = GetDefault<UCXUISettings>()->UIMappingContext.LoadSynchronous();
	if (nullptr == MappingContext)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] UI Mapping Context가 지정되지 않아 키 아이콘을 표시하지 않습니다. (Project Settings > Game > CXVI UI)"), *GetName());
		return;
	}

	FKey KeyboardKey;
	FKey GamepadKey;

	for (const FEnhancedActionKeyMapping& Mapping : MappingContext->GetMappings())
	{
		if (Mapping.Action != HoldAction)
		{
			continue;
		}

		FKey& TargetKey = Mapping.Key.IsGamepadKey() ? GamepadKey : KeyboardKey;
		if (false == TargetKey.IsValid())
		{
			TargetKey = Mapping.Key;
		}
	}

	if (false == KeyboardKey.IsValid() && false == GamepadKey.IsValid())
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] %s의 키가 %s에 매핑되어 있지 않습니다."), *GetName(), *HoldAction->GetName(), *MappingContext->GetName());
	}

	KeyIcon->SetKeys(KeyboardKey, GamepadKey);
}

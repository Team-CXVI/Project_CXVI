// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Field/CXPotionSlotWidget.h"

#include "Foundation/GameplayData/CXGameplayDataSettings.h"
#include "Foundation/UserData/CXUserDataSubsystem.h"
#include "UI/CXUIManagerSubsystem.h"

#include <Components/Image.h>
#include <Materials/MaterialInstanceDynamic.h>
#include <Input/CommonUIInputTypes.h>
#include <InputAction.h>
#include <InputTriggers.h>


#include UE_INLINE_GENERATED_CPP_BY_NAME(CXPotionSlotWidget)

namespace CXPotionSlot
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

	// 설정의 포션 ItemId (지정되지 않으면 None)
	static FName GetPotionItemId()
	{
		return GetDefault<UCXGameplayDataSettings>()->PotionItemId;
	}
}

void UCXPotionSlotWidget::SetIcon(TSoftObjectPtr<UTexture2D> InIcon)
{
	if (nullptr == IconImage)
	{
		return;
	}

	// 지정되지 않은 경우 이전에 표시하던 아이콘이 남지 않도록 비움
	if (InIcon.IsNull())
	{
		IconImage->SetBrushResourceObject(nullptr);
		return;
	}

	IconImage->SetBrushFromSoftTexture(InIcon);
}

void UCXPotionSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Brush에 지정한 머티리얼로 Dynamic Material Instance를 한 번 만들어 Brush에 적용 (이후 파라미터만 갱신)
	HoldGaugeMaterial = HoldGaugeImage ? HoldGaugeImage->GetDynamicMaterial() : nullptr;
	if (nullptr == HoldGaugeMaterial)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] HoldGaugeImage의 Brush에 머티리얼이 지정되지 않아 게이지를 표시하지 않습니다."), *GetName());
	}

	SetGaugePercent(0.f);

	if (nullptr == UsePotionAction)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] UsePotionAction이 지정되지 않았습니다."), *GetName());
		return;
	}

	// 홀드 설정이 빠지면 일반 입력으로 처리되어 누르는 즉시 사용되므로 바인딩하지 않음
	if (false == IsHoldInputConfigured())
	{
		return;
	}

	// HUD(InputMode Game)와 같이 Game 모드에서 입력을 받음
	FBindUIActionArgs BindArgs(UsePotionAction, /*bShouldDisplayInActionBar*/ false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleUsePotion));
	BindArgs.InputMode = ECommonInputMode::Game;
	BindArgs.OnHoldActionProgressed.BindUObject(this, &ThisClass::SetGaugePercent);

	RegisterUIActionBinding(BindArgs);
}

bool UCXPotionSlotWidget::IsHoldInputConfigured() const
{
	bool bConfigured = true;

	const bool bHasHoldTrigger = UsePotionAction->Triggers.ContainsByPredicate([](const TObjectPtr<UInputTrigger>& InTrigger)
		{
			const UInputTriggerHold* HoldTrigger = Cast<UInputTriggerHold>(InTrigger);
			return HoldTrigger && HoldTrigger->HoldTimeThreshold > 0.f;
		});

	if (false == bHasHoldTrigger)
	{
		UE_LOG(LogCXUI, Error, TEXT("[%s] %s에 Hold 트리거(Hold Time Threshold > 0)가 없어 포션 사용 입력을 바인딩하지 않습니다."), *GetName(), *UsePotionAction->GetName());
		bConfigured = false;
	}

	for (const TCHAR* CVarName : { CXPotionSlot::ResolveHoldKeysCVarName, CXPotionSlot::WaitForHoldCompleteCVarName })
	{
		if (false == CXPotionSlot::IsConsoleVariableEnabled(CVarName))
		{
			UE_LOG(LogCXUI, Error, TEXT("[%s] %s=1 이 아니어서 포션 사용 입력을 바인딩하지 않습니다. (Config/DefaultEngine.ini [ConsoleVariables])"), *GetName(), CVarName);
			bConfigured = false;
		}
	}

	return bConfigured;
}

void UCXPotionSlotWidget::SetGaugePercent(float InPercent)
{
	if (HoldGaugeMaterial)
	{
		HoldGaugeMaterial->SetScalarParameterValue(GaugePercentParamName, InPercent);
	}
}

//void UCXPotionSlotWidget::HandleHoldProgressed(float InPercent)
//{
//	// 수량이 0이면 게이지를 채우지 않음 (다 차더라도 RemoveItem에서 다시 막힘)
//	const UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this);
//	const FName PotionItemId = CXPotionSlot::GetPotionItemId();
//	const bool bCanUse = UserData && false == PotionItemId.IsNone() && UserData->GetItemCount(PotionItemId) > 0;
//
//	SetGaugePercent(bCanUse ? InPercent : 0.f);
//}

void UCXPotionSlotWidget::HandleUsePotion()
{
	const FName PotionItemId = CXPotionSlot::GetPotionItemId();
	if (PotionItemId.IsNone())
	{
		UE_LOG(LogCXUI, Warning, TEXT("[%s] PotionItemId가 지정되지 않았습니다. (Project Settings > Game > CXVI Gameplay Data > Item)"), *GetName());
		return;
	}

	UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this);
	if (nullptr == UserData)
	{
		return;
	}

	// 수량이 부족하면 RemoveItem이 빼지 않고 false (위젯 비활성으로 입력이 막히지만 차감 API 결과는 그대로 확인)
	if (false == UserData->RemoveItem(PotionItemId, 1))
	{
		return;
	}

	// TODO : 회복 효과 적용 (필드 = UCXUserDataSubsystem::SetMemberHealth, 대상 / 회복량 미정)
	//	     효과가 실패할 수 있게 되면(전원 최대 HP 등) 차감 전에 판단하도록 순서 재검토



	UE_LOG(LogCXUI, Log, TEXT("Potion used. Remaining: %d"), UserData->GetItemCount(PotionItemId));
}

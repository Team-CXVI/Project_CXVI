// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXDamageNumberComponent.h"

#include "Components/WidgetComponent.h"
#include "Foundation/Combat/CombatMessage/CXCombatMessageData.h"
#include "Tags/CXGameplayTags.h"
#include "UI/CXUIManagerSubsystem.h"
#include "UI/Combat/CXDamageNumberStackWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXDamageNumberComponent)

UCXDamageNumberComponent::UCXDamageNumberComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCXDamageNumberComponent::BeginPlay()
{
	Super::BeginPlay();

	DamageResultListener = UCXGameplayMessageSubsystem::Get(this).RegisterListener(CXVIGameplayTags::Message_Combat_DamageResult, this, &ThisClass::HandleDamageResult);
}

void UCXDamageNumberComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DamageResultListener.Unregister();

	ClearStacks();

	Super::EndPlay(EndPlayReason);
}

void UCXDamageNumberComponent::HandleDamageResult(const FGameplayTag& InChannel, const FCXDamageResultMessage& InMessage)
{
	AActor* Target = InMessage.Target.Get();
	if (!IsValid(Target))
	{
		return;
	}

	if (UCXDamageNumberStackWidget* Stack = FindOrCreateStack(Target))
	{
		Stack->AddDamageResult(InMessage.Result, InMessage.Amount);
	}
}

UCXDamageNumberStackWidget* UCXDamageNumberComponent::FindOrCreateStack(AActor* InTarget)
{
	if (const TWeakObjectPtr<UWidgetComponent>* Found = Stacks.Find(InTarget))
	{
		if (UWidgetComponent* Existing = Found->Get())
		{
			return Cast<UCXDamageNumberStackWidget>(Existing->GetUserWidgetObject());
		}
	}

	if (!StackWidgetClass)
	{
		UE_LOG(LogCXUI, Warning, TEXT("[DamageNumber] StackWidgetClass 없음 (BP_CombatArena의 DamageNumber 컴포넌트 확인)"));
		return nullptr;
	}

	USceneComponent* Anchor = InTarget->FindComponentByTag<USceneComponent>(AnchorTag);
	if (!IsValid(Anchor))
	{
		UE_LOG(LogCXUI, Warning, TEXT("[DamageNumber] %s에 기준점 없음 (Component Tags에 %s 확인)"), *GetNameSafe(InTarget->GetClass()), *AnchorTag.ToString());
		return nullptr;
	}

	// Outer = 대상 → 대상이 Destroy되면 같이 정리
	UWidgetComponent* StackComponent = NewObject<UWidgetComponent>(InTarget);
	StackComponent->SetWidgetSpace(EWidgetSpace::Screen);
	StackComponent->SetWidgetClass(StackWidgetClass);
	StackComponent->SetDrawAtDesiredSize(true);
	StackComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StackComponent->SetGenerateOverlapEvents(false);
	StackComponent->SetupAttachment(Anchor);
	StackComponent->RegisterComponent();

	// 등록 시점에 위젯이 아직 없으면 직접 생성
	if (!StackComponent->GetUserWidgetObject())
	{
		StackComponent->InitWidget();
	}

	Stacks.Add(InTarget, StackComponent);

	return Cast<UCXDamageNumberStackWidget>(StackComponent->GetUserWidgetObject());
}

void UCXDamageNumberComponent::ClearStacks()
{
	for (const TPair<TWeakObjectPtr<AActor>, TWeakObjectPtr<UWidgetComponent>>& Pair : Stacks)
	{
		if (UWidgetComponent* StackComponent = Pair.Value.Get())
		{
			StackComponent->DestroyComponent();
		}
	}

	Stacks.Reset();
}
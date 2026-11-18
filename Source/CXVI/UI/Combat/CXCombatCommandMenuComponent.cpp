// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Combat/CXCombatCommandMenuComponent.h"

#include "Character/Combat/CXCombatCharacterPlayer.h"
#include "Foundation/Combat/CombatMessage/CXCombatMessageData.h"
#include "Tags/CXGameplayTags.h"
#include "UI/CXUIManagerSubsystem.h"
#include "UI/Combat/CXCombatCommandMenuWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatCommandMenuComponent)

UCXCombatCommandMenuComponent::UCXCombatCommandMenuComponent()
{
	SetWidgetSpace(EWidgetSpace::World);
	SetBlendMode(EWidgetBlendMode::Transparent);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);

	// 숨겨져 있다가 표시될 때 최근 렌더 기록이 없어 첫 프레임을 건너뛰지 않도록 항상 그림
	// (건너뛰면 렌더 타깃에 남은 이전 화면이 먼저 보이고, 그동안 패널 애니메이션도 멈춤)
	SetTickWhenOffscreen(true);
}

void UCXCombatCommandMenuComponent::BeginPlay()
{
	Super::BeginPlay();

	InitTransform = GetRelativeTransform();

	// 메시지를 받기 전까지 숨김
	SetVisibility(false);

	CommandMenuListener = UCXGameplayMessageSubsystem::Get(this).RegisterListener(CXVIGameplayTags::Message_Combat_CommandMenuChanged, this, &ThisClass::HandleCommandMenuChanged);
}

void UCXCombatCommandMenuComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CommandMenuListener.Unregister();

	SetRelativeTransform(InitTransform);

	Super::EndPlay(EndPlayReason);
}

void UCXCombatCommandMenuComponent::HandleCommandMenuChanged(const FGameplayTag& InChannel, const FCXCombatCommandMenuMessage& InMessage)
{
	const ACXCombatCharacterPlayer* CommandCharacter = InMessage.CommandCharacter.Get();

	// 조준 모드에서는 숨김. 타겟 선택 화면은 UCXCombatTargetSelectWidget이 담당
	if (!InMessage.bMenuActive || InMessage.bAimMode || InMessage.MenuState == ECommandMenuState::TargetSelect || !IsValid(CommandCharacter))
	{
		SetVisibility(false);
		return;
	}

	const FTransform* Placement = MenuPlacements.Find(InMessage.MenuState);
	UCXCombatCommandMenuWidget* MenuWidget = Cast<UCXCombatCommandMenuWidget>(GetUserWidgetObject());
	if (!Placement || !IsValid(MenuWidget))
	{
		UE_LOG(LogCXUI, Warning, TEXT("[Command Menu] 표시 실패. 배치값=%d, 위젯=%d (BP_CombatArena의 Menu Placements, Widget Class 확인)"),
			Placement != nullptr, IsValid(MenuWidget));
		SetVisibility(false);
		return;
	}

	// 캐릭터 스케일이 메뉴 크기에 영향을 주지 않도록 위치와 방향만 사용
	FTransform CharacterOrigin = CommandCharacter->GetOriginTransform();
	CharacterOrigin.SetScale3D(FVector::OneVector);

	// 숨겨져 있다가 표시되는 경우 (첫 오픈 / 타겟 선택 취소 후 복귀)
	const bool bOpening = !IsVisible();

	SetWorldTransform(InitTransform * *Placement * CharacterOrigin);
	MenuWidget->SetCommandMenu(InMessage.MenuState, CommandCharacter, bOpening);

	// 시작한 패널 애니메이션의 첫 상태를 바로 적용 (평소에는 다음 Slate Tick에 적용되어 한 프레임 늦음)
	MenuWidget->FlushAnimations();

	SetVisibility(true);

	// 렌더 타깃을 지금 갱신해 이전 화면이나 애니메이션 적용 전 화면이 보이지 않게 함
	DrawWidgetToRenderTarget(0.f);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Field/CXFlagMenuWidget.h"

#include "Foundation/Field/FieldEnemySpawning/CXFieldEnemySubsystem.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"
#include "Foundation/UserData/CXUserDataSubsystem.h"
#include "UI/CXUIManagerSubsystem.h"
#include "UI/CXButtonBase.h"
#include "UI/CXUITypes.h"
#include "Tags/CXGameplayTags.h"

#include <Kismet/GameplayStatics.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFlagMenuWidget)

UCXFlagMenuWidget::UCXFlagMenuWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InputMode = ECXWidgetInputMode::Menu;

	// 뒤로 가기 입력처리를 활성화
	bIsBackHandler = true;
}

void UCXFlagMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RestButton->OnClicked().AddUObject(this, &ThisClass::HandleRestClicked);
	SaveButton->OnClicked().AddUObject(this, &ThisClass::HandleSaveClicked);
	ExitButton->OnClicked().AddUObject(this, &ThisClass::HandleExitClicked);
}

void UCXFlagMenuWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	// 메뉴가 열린 동안 플레이어 이동과 적 행동을 멈춤
	UGameplayStatics::SetGamePaused(this, true);
}

void UCXFlagMenuWidget::NativeOnDeactivated()
{
	UGameplayStatics::SetGamePaused(this, false);

	Super::NativeOnDeactivated();
}

void UCXFlagMenuWidget::HandleRestClicked()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UCXFieldEnemySubsystem* EnemySubsystem = World->GetSubsystem<UCXFieldEnemySubsystem>();
	if (!IsValid(EnemySubsystem))
	{
		return;
	}
	const int32 RespawnedCount = EnemySubsystem->RespawnDefeatedEnemies(NAME_None);

	UE_LOG(LogCXUI, Log, TEXT("깃발 휴식: 필드 적 %d 마리 복원"), RespawnedCount);

	UCXUserDataSubsystem& UserData = UCXUserDataSubsystem::Get(this);

	const int32 MemberCount = UserData.GetPartyMembers().Num();
	for (int32 MemberIndex = 0; MemberIndex < MemberCount; ++MemberIndex)
	{
		// 최대 HP를 계산할 수 없으면(데이터 누락) 0이 반환되므로 HP를 0으로 만들지 않도록 건너뜀
		const float MaxHealth = UserData.GetMemberMaxHealth(MemberIndex);
		if (MaxHealth > 0.f)
		{
			UserData.SetMemberHealth(MemberIndex, MaxHealth);
		}
	}
}

void UCXFlagMenuWidget::HandleSaveClicked()
{
	FCXOpenScreenMessage Message;
	Message.ScreenTag = CXVIGameplayTags::UI_Screen_SaveSlot;
	Message.ContextTag = CXVIGameplayTags::UI_ScreenContext_SaveSlot_Save;
	UCXGameplayMessageSubsystem::Get(this).BroadcastMessage(CXVIGameplayTags::Message_UI_OpenScreen, Message);
}

void UCXFlagMenuWidget::HandleExitClicked()
{
	DeactivateWidget();
}

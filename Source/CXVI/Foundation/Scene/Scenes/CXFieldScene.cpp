// Fill out your copyright notice in the Description page of Project Settings.


#include "CXFieldScene.h"
#include "CXCombatScene.h"
#include "CXVI/Foundation/Scene/CXSceneSubsystem.h"
#include "CXVI/Level/CXLevelTransitionSubsystem.h"

#include <GameFramework/Character.h>
#include <GameFramework/CharacterMovementComponent.h>
#include <GameFramework/PlayerController.h>
#include <Kismet/GameplayStatics.h>
#include <EnhancedInputComponent.h>
#include <InputActionValue.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXFieldScene)


bool UCXFieldScene::RequestCombat(UCXSceneDefinition* InCombatScene, FName InEncounterId)
{
	UCXSceneSubsystem* Subsystem = GetSceneSubsystem();
	if (!Subsystem)
	{
		return false;
	}

	FCXCombatEnterParams Params;
	Params.EncounterId = InEncounterId;

	return Subsystem->PushScene(InCombatScene, FCXSceneContext::Make(Params));
}

void UCXFieldScene::OnResume(const FCXSceneContext& InContext)
{
	if (ACharacter* Character = FrozenCharacter.Get())
	{
		Character->SetActorHiddenInGame(false);
		Character->GetCharacterMovement()->SetDefaultMovementMode();
	}
	FrozenCharacter.Reset();

	Super::OnResume(InContext);
}

void UCXFieldScene::OnSuspend()
{
	// TODO : 임시. 필드 Pawn이 ICXSceneSuspendable을 구현하면 제거
	// 필드 레벨이 Hidden 되면 바닥이 사라져 퍼시스턴트의 필드 Pawn이 떨어지므로 숨기고 이동 정지
	// (위 Scene이 다른 Pawn을 빙의하기 전이라 플레이어 캐릭터 = 필드 Pawn)
	if (ACharacter* Character = UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		FrozenCharacter = Character;
		Character->SetActorHiddenInGame(true);
		Character->GetCharacterMovement()->DisableMovement();
	}

	ClearEscapeBinding();

	Super::OnSuspend();
}

void UCXFieldScene::OnExit()
{
	ClearEscapeBinding();

	Super::OnExit();
}

void UCXFieldScene::SetupSceneInput()
{
	Super::SetupSceneInput();

	// TODO : 임시. 일시정지 메뉴가 생기면 제거
	// IA / IMC 에셋 없이 처리하려고 디버그 키 바인딩 사용 (Shipping 빌드에서는 동작하지 않음)
#if !UE_BUILD_SHIPPING
	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	UEnhancedInputComponent* Input = PC ? Cast<UEnhancedInputComponent>(PC->InputComponent) : nullptr;
	if (Input && 0 == EscapeBindingHandle)
	{
		EscapeBindingHandle = Input->BindDebugKey(FInputChord(EKeys::Escape), IE_Pressed, this, &ThisClass::HandleEscapeKey, false).GetHandle();
	}
#endif
}

void UCXFieldScene::HandleEscapeKey(FKey InKey, FInputActionValue InValue)
{
	if (UCXLevelTransitionSubsystem* LevelTransition = UCXLevelTransitionSubsystem::TryGet(this))
	{
		LevelTransition->TravelToTitle();
	}
}

void UCXFieldScene::ClearEscapeBinding()
{
	if (0 == EscapeBindingHandle)
	{
		return;
	}

	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (UEnhancedInputComponent* Input = PC ? Cast<UEnhancedInputComponent>(PC->InputComponent) : nullptr)
	{
		Input->RemoveBindingByHandle(EscapeBindingHandle);
	}

	EscapeBindingHandle = 0;
}

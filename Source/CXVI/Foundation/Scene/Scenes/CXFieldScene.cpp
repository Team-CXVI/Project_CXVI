// Fill out your copyright notice in the Description page of Project Settings.


#include "CXFieldScene.h"
#include "CXCombatScene.h"
#include "CXVI/Foundation/Scene/CXSceneSubsystem.h"

#include <GameFramework/Character.h>
#include <GameFramework/CharacterMovementComponent.h>
#include <Kismet/GameplayStatics.h>

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

	Super::OnSuspend();
}

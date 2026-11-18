// Fill out your copyright notice in the Description page of Project Settings.

#include "CXVictoryStage.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXVictoryStage)

ACXVictoryStage::ACXVictoryStage()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	StageCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("StageCamera"));
	StageCamera->SetupAttachment(Root);
}

void ACXVictoryStage::BeginPlay()
{
	Super::BeginPlay();

	// 에디터에서는 위치를 잡도록 보이고, 게임에서는 Play 전까지 숨기고 충돌도 끈다
	for (ACharacter* Dancer : { DancerA.Get(), DancerB.Get(), BossDancer.Get() })
	{
		if (IsValid(Dancer))
		{
			Dancer->SetActorHiddenInGame(true);
			Dancer->SetActorEnableCollision(false);
		}
	}

	// BP에 붙인 음악(Audio 컴포넌트)은 Play 전까지 멈춰 둔다
	TArray<UAudioComponent*> AudioComps;
	GetComponents<UAudioComponent>(AudioComps);
	for (UAudioComponent* Audio : AudioComps)
	{
		Audio->SetAutoActivate(false);
		Audio->Stop();
	}
}

void ACXVictoryStage::Play()
{
	if (bPlaying)
	{
		return;
	}
	bPlaying = true;

	// 배치된 댄서를 보이게 한다 (춤은 각 댄서 BP의 메시 설정으로 재생)
	for (ACharacter* Dancer : { DancerA.Get(), DancerB.Get(), BossDancer.Get() })
	{
		if (IsValid(Dancer))
		{
			Dancer->SetActorHiddenInGame(false);
		}
	}

	TArray<UAudioComponent*> AudioComps;
	GetComponents<UAudioComponent>(AudioComps);
	for (UAudioComponent* Audio : AudioComps)
	{
		Audio->Play();
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!IsValid(PC))
	{
		return;
	}

	// 필드 플레이어는 숨기고 조작을 막는다
	if (APawn* Pawn = PC->GetPawn())
	{
		Pawn->SetActorHiddenInGame(true);
		Pawn->DisableInput(PC);
	}
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);

	PC->SetViewTargetWithBlend(this, CameraBlendTime);
}

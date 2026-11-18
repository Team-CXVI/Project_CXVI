 // Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Combat/Components/CXWeakPointComponent.h"

#include "Foundation/Combat/CXCollisionChannels.h"

#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXWeakPointComponent)

UCXWeakPointComponent::UCXWeakPointComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	InitSphereRadius(15.f);

	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(CX_TRACE_COMBAT_AIM, ECR_Block);

	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);

}

void UCXWeakPointComponent::BeginPlay()
{
	Super::BeginPlay();

	if (WeakPointSystem && !bBroken)
	{
		WeakPointFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
			WeakPointSystem, this, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset, false);
	}
}

void UCXWeakPointComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(WeakPointFX))
	{
		WeakPointFX->DestroyComponent();
	}
	WeakPointFX = nullptr;

	Super::EndPlay(EndPlayReason);
}

bool UCXWeakPointComponent::TryBreak()
{
	AActor* Owner = GetOwner();

	if (bBroken || !IsValid(Owner) || !Owner->HasAuthority())
	{
		return false;
	}

	// 이벤트 중 다시 호출되어도 중복 파괴되지 않도록 먼저 기록
	bBroken = true;

	// 이후 발사는 뒤에 있는 물체를 맞혀 일반 피해로 처리
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (IsValid(WeakPointFX))
	{
		WeakPointFX->DeactivateImmediate();
	}
	// BP에서 약점 아래에 붙인 표시 메시도 함께 숨긴다.
	SetVisibility(false, true);

	UE_LOG(LogTemp, Display, TEXT("[약점] %s / %s 파괴"),
		*Owner->GetName(),
		*WeakPointId.ToString());

	OnWeakPointBroken.Broadcast(this);

	return true;
}

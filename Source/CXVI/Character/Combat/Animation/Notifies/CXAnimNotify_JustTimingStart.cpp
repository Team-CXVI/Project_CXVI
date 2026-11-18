// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Combat/Animation/Notifies/CXAnimNotify_JustTimingStart.h"

#include "Character/Combat/Components/CXJustTimingComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXAnimNotify_JustTimingStart)

UCXAnimNotify_JustTimingStart::UCXAnimNotify_JustTimingStart(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(180, 165, 255);
#endif
}

void UCXAnimNotify_JustTimingStart::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;

	if (!IsValid(Owner))
	{
		return;
	}

	if (UCXJustTimingComponent* Timing = Owner->FindComponentByClass<UCXJustTimingComponent>())
	{
		Timing->BeginWindow(HitNumber, TimingIndex);
	}
}

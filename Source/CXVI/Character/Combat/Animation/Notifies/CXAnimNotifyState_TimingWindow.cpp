// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Combat/Animation/Notifies/CXAnimNotifyState_TimingWindow.h"

#include "Character/Combat/Components/CXJustTimingComponent.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifyQueue.h"
#include "Animation/AnimTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXAnimNotifyState_TimingWindow)

void UCXAnimNotifyState_TimingWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UAnimMontage* Montage = Cast<UAnimMontage>(Animation);
	const FAnimNotifyEvent* NotifyEvent = EventReference.GetNotify();

	if (!IsValid(Owner) || !IsValid(Montage) || !NotifyEvent)
	{
		return;
	}

	if (UCXJustTimingComponent* Timing = Owner->FindComponentByClass<UCXJustTimingComponent>())
	{
		Timing->BeginWindow(MeshComp, Montage, HitNumber, NotifyEvent->GetTriggerTime(), TotalDuration, SuccessStartRatio, SuccessEndRatio);
	}
}

void UCXAnimNotifyState_TimingWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;

	if (!IsValid(Owner))
	{
		return;
	}

	if (UCXJustTimingComponent* Timing = Owner->FindComponentByClass<UCXJustTimingComponent>())
	{
		Timing->EndWindow(HitNumber);
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "CXAnimNotify_ShowCable.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "../../../../Tags/CXFieldTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXAnimNotify_ShowCable)

void UCXAnimNotify_ShowCable::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, CXGrapplingShowCableTag, FGameplayEventData());
}

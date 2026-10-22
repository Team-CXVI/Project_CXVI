// Fill out your copyright notice in the Description page of Project Settings.


#include "CXAnimNotify_StartGrapplingMove.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "CXVI/Tags/CXFieldTags.h"

void UCXAnimNotify_StartGrapplingMove::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
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

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, CXVIFieldTag::Event_Field_StartGrapplingHook, FGameplayEventData());
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "CXAnimNotify_PreemptiveAttack.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "../../../../Tags/CXFieldTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXAnimNotify_PreemptiveAttack)

void UCXAnimNotify_PreemptiveAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();

	if (nullptr == Owner || !UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		return;
	}

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, CXSlowMotionTag, FGameplayEventData{});
}

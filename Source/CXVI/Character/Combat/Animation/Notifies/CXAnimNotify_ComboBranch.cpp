// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Combat/Animation/Notifies/CXAnimNotify_ComboBranch.h"

#include "Tags/CXGameplayTags.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXAnimNotify_ComboBranch)

void UCXAnimNotify_ComboBranch::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;

	if (!IsValid(Owner) || !UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		return;
	}
	
	FGameplayEventData Payload;
	Payload.EventTag = CXVIGameplayTags::Event_Combat_ComboBranch;
	Payload.Instigator = Owner;
	Payload.EventMagnitude = static_cast<float>(StageNumber);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, CXVIGameplayTags::Event_Combat_ComboBranch, Payload);

}

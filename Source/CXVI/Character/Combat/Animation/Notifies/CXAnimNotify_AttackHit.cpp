// Fill out your copyright notice in the Description page of Project Settings.


#include "CXVI/Character/Combat/Animation/Notifies/CXAnimNotify_AttackHit.h"

#include "CXVI/Tags/CXGameplayTags.h"

#include "AbilitySystemBlueprintLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXAnimNotify_AttackHit)

void UCXAnimNotify_AttackHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;

	if (!IsValid(Owner))
	{
		return;
	}

	// 애니메이션 에디터의 미리보기 액터 등에는 ASC가 없을 수 있음.
	if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = CXVIGameplayTags::Event_Combat_AttackHit;
	Payload.Instigator = Owner;
	Payload.EventMagnitude = static_cast<float>(ComboIndex);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, CXVIGameplayTags::Event_Combat_AttackHit, Payload);
}

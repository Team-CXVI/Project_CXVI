// Fill out your copyright notice in the Description page of Project Settings.


#include "CXAnimNotify_FieldAttackHit.h"
#include "Tags/CXFieldTags.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXAnimNotify_FieldAttackHit)

void UCXAnimNotify_FieldAttackHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp->GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[FieldAttack] 타격 시점 도착"));

	FGameplayEventData Payload;
	Payload.EventTag = CXVIFieldTag::Event_Field_AttackHit;
	Payload.Instigator = Owner;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, CXVIFieldTag::Event_Field_AttackHit, Payload);

	if (!IsValid(MeshComp))
	{
		return;
	}

	UWorld* World = MeshComp->GetWorld();

	// 애니메이션 에디터 미리보기에서는 실행하지 않기.
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	
}

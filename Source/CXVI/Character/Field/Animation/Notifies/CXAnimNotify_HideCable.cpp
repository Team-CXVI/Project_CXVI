 // Fill out your copyright notice in the Description page of Project Settings.


#include "CXAnimNotify_HideCable.h"
#include "../../../../Character/Field/CXFieldPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXAnimNotify_HideCable)

void UCXAnimNotify_HideCable::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	// 기존 애니메이션의 Notify 참조만 유지한다. 케이블 표시는 비활성화했다.
	ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(MeshComp->GetOwner());
	if (nullptr != Player)
	{
		Player->HideCable();
	}
}

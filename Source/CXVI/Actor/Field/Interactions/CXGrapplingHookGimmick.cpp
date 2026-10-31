// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/Field/Interactions/CXGrapplingHookGimmick.h"

#include "Character/Field/CXFieldPlayerCharacter.h"
#include "Interface/CXInteractActionInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXGrapplingHookGimmick)

ACXGrapplingHookGimmick::ACXGrapplingHookGimmick()
{


}

void ACXGrapplingHookGimmick::Interact_Implementation(AActor* Interactor)
{
	if (nullptr == GrapplingTarget || nullptr == LandingTarget)
	{
		UE_LOG(LogCXGimmic, Error, TEXT("Interact Failed(GrapplingHook): GrapplingTarget 또는 LandingTarget가 없습니다."));
		return;
	}

	if (!Interactor->Implements<UCXInteractActionInterface>())
	{
		return;
	}

	FCXInteractActionParam Param = FCXInteractActionParam::Make<FCXGrapplingHookActionParams>({ GrapplingTarget, LandingTarget });

	ICXInteractActionInterface::Execute_StartInteractAction(Interactor, EInteractActionType::GrapplingHook, Param);

}

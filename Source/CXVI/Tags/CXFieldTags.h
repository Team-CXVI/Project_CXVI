#pragma once

#include "NativeGameplayTags.h"


namespace CXVIFieldTag
{
	// Status
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_MovingBlock);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_CanGrapplingHook);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_CanAssassination);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_CanUseExpeditionFlag);

	// Input
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Field_Sprint);

	// Evnet
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Field_EnterSlowMotion);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Field_ShowCable);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Field_StartGrapplingHook);
}


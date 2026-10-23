// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

namespace CXVIGameplayTags
{
	// Input
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Combat_Confirm);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Combat_Parry);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Combat_Dodge);

	// Status
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Stunned);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Dead);

	// Event
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Combat_ParrySucceeded);

	// Message
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Combat_End);

	// Data
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);

	// UI Layer
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Game);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_GameMenu);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Menu);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Modal);
}